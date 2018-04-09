/**
 * DeepDetect
 * Copyright (c) 2016 Emmanuel Benazera
 * Author: Emmanuel Benazera <beniz@droidnik.fr>
 *
 * This file is part of deepdetect.
 *
 * deepdetect is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * deepdetect is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with deepdetect.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SUPERVISEDOUTPUTCONNECTOR_H
#define SUPERVISEDOUTPUTCONNECTOR_H

namespace dd
{

    /**
   * \brief supervised machine learning output connector class
   */
  class SupervisedOutput : public OutputConnectorStrategy
  {
  public:

    /**
     * \brief supervised result
     */
    class sup_result
    {
    public:
      /**
       * \brief constructor
       * @param loss result loss
       */
      sup_result(const std::string &label, const double &loss=0.0)
	:_label(label),_loss(loss) {}
      
      /**
       * \brief destructor
       */
      ~sup_result() {}

      /**
       * \brief add category to result
       * @param prob category predicted probability
       * @Param cat category name
       */
      inline void add_cat(const double &prob, const std::string &cat)
      {
	_cats.insert(std::pair<double,std::string>(prob,cat));
      }

      inline void add_bbox(const double &prob, const APIData &ad)
      {
        _bboxes.insert(std::pair<double,APIData>(prob,ad));
      }

      inline void add_val(const double &prob, const APIData &ad)
      {
        _vals.insert(std::pair<double,APIData>(prob,ad));
      }

#ifdef USE_SIMSEARCH
      void add_nn(const double &dist, const URIData &uri)
      {
	_nns.insert(std::pair<double,URIData>(dist,uri));
      }
      
      void add_bbox_nn(const int &bb, const double &dist, const URIData &uri)
      {
	if (_bbox_nns.empty())
	  _bbox_nns = std::vector<std::multimap<double,URIData>>(_bboxes.size(),std::multimap<double,URIData>());
	_bbox_nns.at(bb).insert(std::pair<double,URIData>(dist,uri));
      }
#endif

      std::string _label;
      double _loss = 0.0; /**< result loss. */
      std::multimap<double,std::string,std::greater<double>> _cats; /**< categories and probabilities for this result */
      std::multimap<double,APIData,std::greater<double>> _bboxes; /**< bounding boxes information */
      std::multimap<double,APIData,std::greater<double>> _vals; /**< extra data or information added to output, e.g. roi */
#ifdef USE_SIMSEARCH
      bool _indexed = false;
      std::multimap<double,URIData> _nns; /**< nearest neigbors. */
      std::vector<std::multimap<double,URIData>> _bbox_nns; /**< per bbox nearest neighbors. */
#endif      
    };

  public:
    /**
     * \brief supervised output connector constructor
     */
  SupervisedOutput()
      :OutputConnectorStrategy()
      {
      }
    
    /**
     * \brief supervised output connector copy-constructor
     * @param sout supervised output connector
     */
    SupervisedOutput(const SupervisedOutput &sout)
      :OutputConnectorStrategy(),_best(sout._best)
      {
      }
    
    ~SupervisedOutput() {}

    /**
     * \brief supervised output connector initialization
     * @param ad data object for "parameters/output"
     */
    void init(const APIData &ad)
    {
      APIData ad_out = ad.getobj("parameters").getobj("output");
      if (ad_out.has("best"))
	_best = ad_out.get("best").get<int>();
      if (_best == -1)
	_best = ad_out.get("nclasses").get<int>();
    }

    /**
     * \brief add prediction result to supervised connector output
     * @param vrad vector of data objects
     */
    inline void add_results(const std::vector<APIData> &vrad)
    {
      std::unordered_map<std::string,int>::iterator hit;
      for (APIData ad: vrad)
	{ 
	  std::string uri = ad.get("uri").get<std::string>();
	  double loss = ad.get("loss").get<double>();
	  std::vector<double> probs = ad.get("probs").get<std::vector<double>>();
	  std::vector<std::string> cats = ad.get("cats").get<std::vector<std::string>>();
	  std::vector<APIData> bboxes;
	  std::vector<APIData> rois;
	  if (ad.has("bboxes"))
	    bboxes = ad.getv("bboxes");
	  if (ad.has("vals")) {
	    rois = ad.getv("vals");
	  }
	  if ((hit=_vcats.find(uri))==_vcats.end())
	    {
	      auto resit = _vcats.insert(std::pair<std::string,int>(uri,_vvcats.size()));
	      _vvcats.push_back(sup_result(uri,loss));
	      hit = resit.first;
	      for (size_t i=0;i<probs.size();i++)
		{
		  _vvcats.at((*hit).second).add_cat(probs.at(i),cats.at(i));
		  if (!bboxes.empty())
		    _vvcats.at((*hit).second).add_bbox(probs.at(i),bboxes.at(i));
		  if (!rois.empty())
		    _vvcats.at((*hit).second).add_val(probs.at(i),rois.at(i));
		}
	    }
	}
    }
    
    /**
     * \brief best categories selection from results
     * @param ad_out output data object
     * @param bcats supervised output connector
     */
    void best_cats(const APIData &ad_out, SupervisedOutput &bcats, const int &nclasses, const bool &has_bbox, const bool &has_roi) const
    {
      int best = _best;
      if (ad_out.has("best"))
	best = ad_out.get("best").get<int>();
      if (best == -1)
	best = nclasses;
      if (!has_bbox && !has_roi)
	{
	  for (size_t i=0;i<_vvcats.size();i++)
	    {
	      sup_result sresult = _vvcats.at(i);
	      sup_result bsresult(sresult._label,sresult._loss);
	      std::copy_n(sresult._cats.begin(),std::min(best,static_cast<int>(sresult._cats.size())),
			  std::inserter(bsresult._cats,bsresult._cats.end()));
	      if (!sresult._bboxes.empty())
		std::copy_n(sresult._bboxes.begin(),std::min(best,static_cast<int>(sresult._bboxes.size())),
			    std::inserter(bsresult._bboxes,bsresult._bboxes.end()));
	      if (!sresult._vals.empty())
		std::copy_n(sresult._vals.begin(),std::min(best,static_cast<int>(sresult._vals.size())),
			    std::inserter(bsresult._vals,bsresult._vals.end()));
	      
	      bcats._vcats.insert(std::pair<std::string,int>(sresult._label,bcats._vvcats.size()));
	      bcats._vvcats.push_back(bsresult);
	    }
	}
      else
	{
	  for (size_t i=0;i<_vvcats.size();i++)
	    {
	      sup_result sresult = _vvcats.at(i);
	      sup_result bsresult(sresult._label,sresult._loss);

	      if (best == nclasses)
		{
		  int nbest = sresult._cats.size();
		  std::copy_n(sresult._cats.begin(),std::min(nbest,static_cast<int>(sresult._cats.size())),
			      std::inserter(bsresult._cats,bsresult._cats.end()));
		  if (!sresult._bboxes.empty())
		    std::copy_n(sresult._bboxes.begin(),std::min(nbest,static_cast<int>(sresult._bboxes.size())),
				std::inserter(bsresult._bboxes,bsresult._bboxes.end()));
		}
	      else
		{
		  std::unordered_map<std::string,int> lboxes;
		  auto bbit = lboxes.begin();
		  auto mit = sresult._cats.begin();
		  auto mitx = sresult._bboxes.begin();
		  auto mity = sresult._vals.begin();
		  while(mitx!=sresult._bboxes.end())
		    {
		      APIData bbad = (*mitx).second;
		      APIData vvad;
		      if (has_roi)
			vvad = (*mity).second;
		      std::string bbkey = std::to_string(bbad.get("xmin").get<double>())
			+ "-" + std::to_string(bbad.get("ymin").get<double>())
			+ "-" + std::to_string(bbad.get("xmax").get<double>())
			+ "-" + std::to_string(bbad.get("ymax").get<double>());
		      if ((bbit=lboxes.find(bbkey))!=lboxes.end())
			{
			  (*bbit).second += 1;
			  if ((*bbit).second <= best)
			    {
			      bsresult._cats.insert(std::pair<double,std::string>((*mit).first,(*mit).second));
			      bsresult._bboxes.insert(std::pair<double,APIData>((*mitx).first,bbad));
			      if (has_roi)
				bsresult._vals.insert(std::pair<double,APIData>((*mity).first,vvad));
			    }
			}
		      else
			{
			  lboxes.insert(std::pair<std::string,int>(bbkey,1));
			  bsresult._cats.insert(std::pair<double,std::string>((*mit).first,(*mit).second));
			  bsresult._bboxes.insert(std::pair<double,APIData>((*mitx).first,bbad));
			  if (has_roi)
			    bsresult._vals.insert(std::pair<double,APIData>((*mity).first,vvad));
			}
		      ++mitx;
		      ++mit;
		      if (has_roi)
			++mity;
		    }
		}
	      bcats._vcats.insert(std::pair<std::string,int>(sresult._label,bcats._vvcats.size()));
	      bcats._vvcats.push_back(bsresult);
	    }
	}
    }

    /**
     * \brief finalize output supervised connector data
     * @param ad_in data output object from the API call
     * @param ad_out data object as the call response
     */
    void finalize(const APIData &ad_in, APIData &ad_out, MLModel *mlm)
    {
      SupervisedOutput bcats(*this);
      bool regression = false;
      bool autoencoder = false;
      int nclasses = -1;
      if (ad_out.has("nclasses"))
	nclasses = ad_out.get("nclasses").get<int>();
      if (ad_out.has("regression"))
	{
	  if (ad_out.get("regression").get<bool>())
	    {
	      regression = true;
	      _best = ad_out.get("nclasses").get<int>();
	    }
	  ad_out.erase("regression");
	  ad_out.erase("nclasses");
	}
      if (ad_out.has("autoencoder") && ad_out.get("autoencoder").get<bool>())
	{
	  autoencoder = true;
	  _best = 1;
	  ad_out.erase("autoencoder");
	}
      bool has_bbox = false;
      bool has_roi = false;
      if (ad_out.has("bbox") && ad_out.get("bbox").get<bool>())
	{
	  has_bbox = true;
	  ad_out.erase("nclasses");
	  ad_out.erase("bbox");
	}

      if (ad_out.has("roi") && ad_out.get("roi").get<bool>())
        {
          has_roi = true;
        }

      best_cats(ad_in,bcats,nclasses,has_bbox,has_roi);
      
      std::unordered_set<std::string> indexed_uris;
#ifdef USE_SIMSEARCH
      // index
      if (ad_in.has("index") && ad_in.get("index").get<bool>())
	{
	  // check whether index has been created
	  if (!mlm->_se)
	    {
	      int index_dim = _best;
	      if (has_roi)
		index_dim = (*bcats._vvcats.at(0)._vals.begin()).second.get("vals").get<std::vector<double>>().size(); // lookup to the first roi dimensions
	      mlm->create_sim_search(index_dim);
	    }

	  // index output content
	  if (!has_roi)
	    {
	      for (size_t i=0;i<bcats._vvcats.size();i++)
		{
		  std::vector<double> probs;
		  auto mit = bcats._vvcats.at(i)._cats.begin();
		  while(mit!=bcats._vvcats.at(i)._cats.end())
		    {
		      probs.push_back((*mit).first);
		      ++mit;
		    }
		  URIData urid(bcats._vvcats.at(i)._label);
		  mlm->_se->index(urid,probs);
		  indexed_uris.insert(urid._uri);
		}
	    }
	  else // roi
	    {
	      int nrois = 0;
	      for (size_t i=0;i<bcats._vvcats.size();i++)
		{
		  auto vit = bcats._vvcats.at(i)._vals.begin();
		  auto bit = bcats._vvcats.at(i)._bboxes.begin();
		  auto mit = bcats._vvcats.at(i)._cats.begin();
		  while(mit!=bcats._vvcats.at(i)._cats.end())
		    {
		      std::vector<double> bbox = {(*bit).second.get("xmin").get<double>(),
						  (*bit).second.get("ymin").get<double>(),
						  (*bit).second.get("xmax").get<double>(),
						  (*bit).second.get("ymax").get<double>()};
		      double prob = (*mit).first;
		      std::string cat = (*mit).second;
		      URIData urid(bcats._vvcats.at(i)._label,
				   bbox,prob,cat);
		      mlm->_se->index(urid,(*vit).second.get("vals").get<std::vector<double>>());
		      ++mit;
		      ++vit;
		      ++bit;
		      ++nrois;
		      indexed_uris.insert(urid._uri);
		    }
		}
	    }
	}
      
      // build index
      if (ad_in.has("build_index") && ad_in.get("build_index").get<bool>())
	{
	  if (mlm->_se)
	    mlm->build_index();
	  else throw SimIndexException("Cannot build index if not created");
	}

      // search
      if (ad_in.has("search") && ad_in.get("search").get<bool>())
	{
	  // check whether index has been created
	  if (!mlm->_se)
	    {
	      int index_dim = _best;
	      if (has_roi && !bcats._vvcats.at(0)._vals.empty())
		{
		  index_dim = (*bcats._vvcats.at(0)._vals.begin()).second.get("vals").get<std::vector<double>>().size(); // lookup to the first roi dimensions
		  mlm->create_sim_search(index_dim);
		}
	    }
	  
	  int search_nn = _best;
	  if (has_roi)
	    search_nn = _search_nn;
	  if (ad_in.has("search_nn"))
	    search_nn = ad_in.get("search_nn").get<int>();
	  if (!has_roi)
	    {
	      for (size_t i=0;i<bcats._vvcats.size();i++)
		{
		  std::vector<double> probs;
		  auto mit = bcats._vvcats.at(i)._cats.begin();
		  while(mit!=bcats._vvcats.at(i)._cats.end())
		    {
		      probs.push_back((*mit).first);
		      ++mit;
		    }
		  std::vector<URIData> nn_uris;
		  std::vector<double> nn_distances;
		  mlm->_se->search(probs,search_nn,nn_uris,nn_distances);
		  for (size_t j=0;j<nn_uris.size();j++)
		    {
		      bcats._vvcats.at(i).add_nn(nn_distances.at(j),nn_uris.at(j));
		    }
		}
	    }
	  else // has_roi
	    {
	      for (size_t i=0;i<bcats._vvcats.size();i++)
		{
		  int bb = 0;
		  auto vit = bcats._vvcats.at(i)._vals.begin();
		  auto mit = bcats._vvcats.at(i)._cats.begin(); 
		  while(mit!=bcats._vvcats.at(i)._cats.end()) // equivalent to iterating the bboxes
		    {
		      std::vector<URIData> nn_uris;
		      std::vector<double> nn_distances;
		      mlm->_se->search((*vit).second.get("vals").get<std::vector<double>>(),
				       search_nn,nn_uris,nn_distances);
		      ++mit;
		      ++vit;
		      for (size_t j=0;j<nn_uris.size();j++)
			{
			  bcats._vvcats.at(i).add_bbox_nn(bb,nn_distances.at(j),nn_uris.at(j));
			}
		      ++bb;
		    }
		}
	    }
	}      
#endif
      
      bcats.to_ad(ad_out,regression,autoencoder,has_bbox,has_roi,indexed_uris);
    }
    
    struct PredictionAndAnswer {
      float prediction;
      unsigned char answer; //this is either 0 or 1
    };

    // measure
    static void measure(const APIData &ad_res, const APIData &ad_out, APIData &out)
    {
      APIData meas_out;
      bool tloss = ad_res.has("train_loss");
      bool loss = ad_res.has("loss");
      bool iter = ad_res.has("iteration");
      bool regression = ad_res.has("regression");
      bool segmentation = ad_res.has("segmentation");
      bool multilabel = ad_res.has("multilabel");
      if (ad_out.has("measure"))
	{
	  std::vector<std::string> measures = ad_out.get("measure").get<std::vector<std::string>>();
      	  bool bauc = (std::find(measures.begin(),measures.end(),"auc")!=measures.end());
	  bool bacc = false;

	  if (!multilabel && !segmentation)
	    {
	      for (auto s: measures)
		if (s.find("acc")!=std::string::npos)
		  {
		    bacc = true;
		    break;
		  }
	    }
	  bool bf1 = (std::find(measures.begin(),measures.end(),"f1")!=measures.end());
	  bool bmcll = (std::find(measures.begin(),measures.end(),"mcll")!=measures.end());
	  bool bgini = (std::find(measures.begin(),measures.end(),"gini")!=measures.end());
	  bool beucll = (std::find(measures.begin(),measures.end(),"eucll")!=measures.end());
	  bool bmcc = (std::find(measures.begin(),measures.end(),"mcc")!=measures.end());
	  bool baccv = false;
	  bool mlacc = false;
      bool mlsoft = false;
       if (segmentation)
	    baccv = (std::find(measures.begin(),measures.end(),"acc")!=measures.end());
	  if (multilabel && !regression)
	    mlacc = (std::find(measures.begin(),measures.end(),"acc")!=measures.end());

      if (multilabel && regression)
        {
          mlsoft = (std::find(measures.begin(),measures.end(),"acc")!=measures.end());
        }
	  if (bauc) // XXX: applies two binary classification problems only
	    {
	      double mauc = auc(ad_res);
	      meas_out.add("auc",mauc);
	    }
	  if (bacc)
	    {
	      std::map<std::string,double> accs = acc(ad_res,measures);
	      auto mit = accs.begin();
	      while(mit!=accs.end())
		{
		  meas_out.add((*mit).first,(*mit).second);
		  ++mit;
		}
	    }
	  if (baccv)
	    {
	      double meanacc, meaniou;
	      std::vector<double> clacc;
	      double accs = acc_v(ad_res,meanacc,meaniou,clacc);
	      meas_out.add("acc",accs);
	      meas_out.add("meanacc",meanacc);
	      meas_out.add("meaniou",meaniou);
	      meas_out.add("clacc",clacc);
	    }
	  if (mlacc)
	    {
	      double f1, sensitivity, specificity, harmmean, precision;
	      multilabel_acc(ad_res,sensitivity,specificity,harmmean,precision,f1);
	      meas_out.add("f1",f1);
	      meas_out.add("precision",precision);
	      meas_out.add("sensitivity",sensitivity);
	      meas_out.add("specificity",specificity);
	      meas_out.add("harmmean",harmmean);
	    }
      if (mlsoft)
        {
          // below measures for soft multilabel
          double kl_divergence; // kl: amount of lost info if using pred instead of truth
          double js_divergence; // jsd: symetrized version of kl
          double wasserstein; // wasserstein distance
          double kolmogorov_smirnov; // kolmogorov-smirnov test aka max individual error
          double distance_correlation; // distance correlation , same as brownian correlation
          double r_2; // r_2 score: best  is 1, min is 0
          double delta_scores[4]; // delta-score , aka 1 if pred \in [truth-delta, truth+delta]
          double deltas[4] = {0.05, 0.1, 0.2, 0.5};
          multilabel_acc_soft(ad_res, kl_divergence, js_divergence, wasserstein, kolmogorov_smirnov,
                               distance_correlation, r_2, delta_scores, deltas, 4);
          meas_out.add("kl_divergence",kl_divergence);
	      meas_out.add("js_divergence",js_divergence);
	      meas_out.add("wasserstein",wasserstein);
	      meas_out.add("kolmogorov_smirnov",kolmogorov_smirnov);
	      meas_out.add("distance_correlation",distance_correlation);
          meas_out.add("r2",r_2);
          for (int i=0; i<4; ++i)
            {
              std::ostringstream sstr;
              sstr << "delta_score_" << deltas[i];
              meas_out.add(sstr.str(),delta_scores[i]);
            }
        }
	  if (!multilabel && !segmentation && bf1)
	    {
	      double f1,precision,recall,acc;
	      dMat conf_diag,conf_matrix;
	      f1 = mf1(ad_res,precision,recall,acc,conf_diag,conf_matrix);
	      meas_out.add("f1",f1);
	      meas_out.add("precision",precision);
	      meas_out.add("recall",recall);
	      meas_out.add("accp",acc);
	      if (std::find(measures.begin(),measures.end(),"cmdiag")!=measures.end())
		{
		  std::vector<double> cmdiagv;
		  for (int i=0;i<conf_diag.rows();i++)
		    cmdiagv.push_back(conf_diag(i,0));
		  meas_out.add("cmdiag",cmdiagv);
		  meas_out.add("labels",ad_res.get("clnames").get<std::vector<std::string>>());
		}
	      if (std::find(measures.begin(),measures.end(),"cmfull")!=measures.end())
		{
		  std::vector<std::string> clnames = ad_res.get("clnames").get<std::vector<std::string>>();
		  std::vector<APIData> cmdata;
		  for (int i=0;i<conf_matrix.cols();i++)
		    {
		      std::vector<double> cmrow;
		      for (int j=0;j<conf_matrix.rows();j++)
			cmrow.push_back(conf_matrix(j,i));
		      APIData adrow;
		      adrow.add(clnames.at(i),cmrow);
		      cmdata.push_back(adrow);
		    }
		  meas_out.add("cmfull",cmdata);
		}
	    }
	  if (!multilabel && !segmentation && bmcll)
	    {
	      double mmcll = mcll(ad_res);
	      meas_out.add("mcll",mmcll);
	    }
	  if (bgini)
	    {
	      double mgini = gini(ad_res,regression);
	      meas_out.add("gini",mgini);
	    }
	  if (beucll)
	    {
	      double meucll = eucll(ad_res);
	      meas_out.add("eucll",meucll);
	    }
	  if (bmcc)
	    {
	      double mmcc = mcc(ad_res);
	      meas_out.add("mcc",mmcc);
	      
	    }
	}
	if (loss)
	  meas_out.add("loss",ad_res.get("loss").get<double>()); // 'universal', comes from algorithm
	if (tloss)
	  meas_out.add("train_loss",ad_res.get("train_loss").get<double>());
	if (iter)
	  meas_out.add("iteration",ad_res.get("iteration").get<double>());
	out.add("measure",meas_out);
    }

    // measure: ACC
    static std::map<std::string,double> acc(const APIData &ad,
					    const std::vector<std::string> &measures)
    {
      struct acc_comp
      {
        acc_comp(const std::vector<double> &v)
	:_v(v) {}
	bool operator()(double a, double b) { return _v[a] > _v[b]; }
	const std::vector<double> _v;
      };
      std::map<std::string,double> accs;
      std::vector<int> vacck;
      for(auto s: measures)
	if (s.find("acc")!=std::string::npos)
	  {
	    std::vector<std::string> sv = dd_utils::split(s,'-');
	    if (sv.size() == 2)
	      {
		vacck.push_back(std::atoi(sv.at(1).c_str()));
	      }
	    else vacck.push_back(1);
	  }
      
      int batch_size = ad.get("batch_size").get<int>();
      for (auto k: vacck)
	{
	  double acc = 0.0;
	  for (int i=0;i<batch_size;i++)
	    {
	      APIData bad = ad.getobj(std::to_string(i));
	      std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	      if (k-1 >= static_cast<int>(predictions.size()))
		continue; // ignore instead of error
	      std::vector<int> predk(predictions.size());
	      for (size_t j=0;j<predictions.size();j++)
		predk[j] = j;
	      std::partial_sort(predk.begin(),predk.begin()+k-1,predk.end(),acc_comp(predictions));
	      for (int l=0;l<k;l++)
		if (predk.at(l) == bad.get("target").get<double>())
		  {
		    acc++;
		    break;
		  }
	    }
	  std::string key = "acc";
	  if (k>1)
	    key += "-" + std::to_string(k);
	  accs.insert(std::pair<std::string,double>(key,acc / static_cast<double>(batch_size)));
	}
      return accs;
    }

    static double acc_v(const APIData &ad, double &meanacc, double &meaniou, std::vector<double> &clacc)
    {
      int nclasses = ad.get("nclasses").get<int>();
      int batch_size = ad.get("batch_size").get<int>();
      std::vector<double> mean_acc(nclasses,0.0);
      std::vector<double> mean_acc_bs(nclasses,0.0);
      std::vector<double> mean_iou_bs(nclasses,0.0);
      std::vector<double> mean_iou(nclasses,0.0);
      double acc_v = 0.0;
      meanacc = 0.0;
      meaniou = 0.0;
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>(); // all best-1
	  std::vector<double> targets = bad.get("target").get<std::vector<double>>(); // all targets against best-1
	  dVec dpred = dVec::Map(&predictions.at(0),predictions.size());
	  dVec dtarg = dVec::Map(&targets.at(0),targets.size());
	  dVec ddiff = dpred - dtarg;
	  double acc = (ddiff.cwiseAbs().array() == 0).count() / static_cast<double>(dpred.size());
	  acc_v += acc;
	  
	  for (int c=0;c<nclasses;c++)
	    {
	      dVec dpredc = (dpred.array() == c).select(dpred,dVec::Constant(dpred.size(),-2.0));
	      dVec dtargc = (dtarg.array() == c).select(dtarg,dVec::Constant(dtarg.size(),-1.0));
	      dVec ddiffc = dpredc - dtargc;
	      double c_sum = (ddiffc.cwiseAbs().array() == 0).count();

	      // mean acc over classes
	      double c_total_targ = static_cast<double>((dtarg.array() == c).count());
	      if (c_sum == 0 || c_total_targ == 0)
		{}
	      else
		{
		  double accc = c_sum / c_total_targ;
		  mean_acc[c] += accc;
		  mean_acc_bs[c]++;
		}

	      // mean intersection over union
	      double c_false_neg = static_cast<double>((ddiffc.array() == -2-c).count());
	      double c_false_pos = static_cast<double>((ddiffc.array() == c+1).count()); 
	      double iou = c_sum / (c_false_pos + c_sum + c_false_neg);
	      mean_iou[c] += iou;
	      mean_iou_bs[c]++;
	    }
	}
      int c_nclasses = 0;
      for (int c=0;c<nclasses;c++)
	{
	  if (mean_acc_bs[c] > 0.0)
	    {
	      mean_acc[c] /= mean_acc_bs[c];
	      mean_iou[c] /= mean_iou_bs[c];
	      c_nclasses++;
	    }
	  meanacc += mean_acc[c];
	  meaniou += mean_iou[c];
	}
      clacc = mean_acc;
      meanacc /= static_cast<double>(c_nclasses);
      meaniou /= static_cast<double>(c_nclasses);
      return acc_v / static_cast<double>(batch_size);
    }

    // multilabel measures
    static double multilabel_acc(const APIData &ad, double &sensitivity, double &specificity,
				 double &harmmean, double &precision, double &f1)
    {
      int batch_size = ad.get("batch_size").get<int>();
      double tp = 0.0;
      double fp = 0.0;
      double tn = 0.0;
      double fn = 0.0;
      double count_pos = 0.0;
      double count_neg = 0.0;
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> targets = bad.get("target").get<std::vector<double>>();
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	  for (size_t j=0;j<predictions.size();j++)
	    {
	      if (targets.at(j) < 0)
		continue;
	      if (targets.at(j) >= 0.5)
		{
		  // positive accuracy
		  if (predictions.at(j) >= 0)
		    ++tp;
		  else ++fn;
		  ++count_pos;
		}
	      else
		{
		  // negative accuracy
		  if (predictions.at(j) < 0)
		    ++tn;
		  else ++fp;
		  ++count_neg;
		}
	    }
	}
      sensitivity = (count_pos > 0) ? tp / count_pos : 0.0;
      specificity = (count_neg > 0) ? tn / count_neg : 0.0;
      harmmean = ((count_pos + count_neg > 0)) ?
	2 / (count_pos / tp + count_neg / tn) : 0.0;
      precision = (tp > 0) ? (tp / (tp + fp)): 0.0;
      f1 = (tp > 0) ? 2 * tp / (2 * tp + fp + fn): 0.0;
      return f1;
    }

    static double multilabel_acc_soft(const APIData &ad, double &kl_divergence, double  &js_divergence,
                                      double &wasserstein,
                                      double &kolmogorov_smirnov,double &distance_correlation,
                                      double &r_2,
                                      double *delta_scores, const double deltas[], const int ndeltas)
    {
      int batch_size = ad.get("batch_size").get<int>();
      kl_divergence = 0;
      js_divergence = 0;
      wasserstein = 0;
      kolmogorov_smirnov = 0;
      long int total_number = 0;
      double tmean = 0;
      for (int k =0; k<ndeltas; ++k)
        delta_scores[k] = 0;
      for (int i=0;i<batch_size;i++)
        {
          APIData bad = ad.getobj(std::to_string(i));
          std::vector<double> targets = bad.get("target").get<std::vector<double>>();
          std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
          for (size_t j=0;j<predictions.size();j++)
            {
              if (targets[j] < 0) // case ignore_label
                 continue;
              total_number++;
              // d_kl(target||pred) = sum target * log (target/pred)
              // do not work if zeros, give a threshold
              double eps = 0.00001;
              double tval = (targets[j]< eps)? eps: targets[j];
              double pval = (predictions[j]< eps)? eps: predictions[j];
              kl_divergence += tval * log (tval/pval);
              js_divergence += 0.5 * (tval * log(2*tval/(tval+pval))) + 0.5 * (pval * log(2*pval/(tval+pval)));
              double dif = targets[j] - predictions[j];
              wasserstein += dif*dif;
              dif = fabs(dif);
              if (dif > kolmogorov_smirnov)
                kolmogorov_smirnov = dif;
              for (int k=0; k<ndeltas; ++k)
                  if (dif < deltas[k])
                    delta_scores[k]++;
              tmean += targets[j];
            }
        }


      double ssres = wasserstein;
      wasserstein = sqrt(wasserstein);
      // below normalize to compare different learnings
      kl_divergence /= (double)total_number;
      js_divergence /= (double)total_number;
      wasserstein /=  sqrt((double)total_number); // gives distance between 0 and 1
      for (int k =0; k<ndeltas; ++k)
        delta_scores[k] /=  (double)total_number; // gives proportion of good in 0:1 at every threshold
      tmean /= (double)total_number;

      double sstot = 0;

      for (int i=0;i<batch_size;i++)
        {
          APIData bad = ad.getobj(std::to_string(i));
          std::vector<double> targets = bad.get("target").get<std::vector<double>>();
          std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
          for (size_t j=0;j<predictions.size();j++)
            {
              if (targets[j] < 0)
                continue;
              sstot += (targets[j] - tmean) *  (targets[j] - tmean);
            }
        }

      int nclasses = ad.getobj(std::to_string(0)).get("target").get<std::vector<double>>().size();

      distance_correlation = 0;



      double t_jk[nclasses][nclasses];
      double p_jk[nclasses][nclasses];
      double t_j[nclasses];
      double t_k[nclasses];
      double p_j[nclasses];
      double p_k[nclasses];
      double t_;
      double p_;

      for (int i =0; i< batch_size; ++i) {
        APIData badj = ad.getobj(std::to_string(i));
        std::vector<double> targets = badj.get("target").get<std::vector<double>>();
        std::vector<double> predictions = badj.get("pred").get<std::vector<double>>();


        for (int j=0;j<nclasses;j++)
          for (int k=0; k<nclasses; ++k)
            {
              if (targets[j] < 0 || targets[k] < 0)
                continue;
              p_jk[j][k] = sqrt((predictions[j]-predictions[k]) * (predictions[j] -predictions[k])) ;
              t_jk[j][k] = sqrt((targets[j]-targets[k]) * (targets[j] -targets[k])) ;;
            }

        t_ = 0;
        p_ = 0;
        for (int l =0; l<nclasses; ++l) {
          t_j[l] = 0;
          t_k[l] = 0;
          p_j[l] = 0;
          p_k[l] = 0;
          for (int m =0; m<nclasses; ++m) {
            t_j[m] += t_jk[l][m];
            t_k[m] += t_jk[m][l];
            p_j[m] += p_jk[l][m];
            p_k[m] += p_jk[m][l];
          }
          t_j[l] /= (double)nclasses;
          t_ += t_j[l];
          t_k[i] /= (double)nclasses;
          p_j[i] /= (double)nclasses;
          p_ += p_j[i];
          p_k[i] /= (double)nclasses;
        }
        t_ /= (double) nclasses;
        p_ /= (double) nclasses;

        double dcov = 0;
        double dvart = 0;
        double dvarp = 0;

        for (int j=0; j<nclasses; ++j)
          for (int k=0; k<nclasses; ++k)
          {
            double p = p_jk[j][k] - p_j[j] - p_k[k] + p_;
            double t = t_jk[j][k] - t_j[j] - t_k[k] + t_;
            dcov += p*t;
            dvart += t*t;
            dvarp += p*p;
          }
        dcov /= nclasses * nclasses;
        dvart /= nclasses* nclasses;
        dvarp /= nclasses * nclasses;
        dcov = sqrt(dcov);
        dvart = sqrt(dvart);

        dvarp = sqrt(dvarp);

        if (dvart != 0 && dvarp !=0)
          distance_correlation += dcov / (sqrt(dvart * dvarp));
      }
      distance_correlation /= batch_size;

      r_2 = 1.0 - ssres/sstot;
      return r_2;
    }


    // measure: F1
    static double mf1(const APIData &ad, double &precision, double &recall, double &acc, dMat &conf_diag, dMat &conf_matrix)
    {
      int nclasses = ad.get("nclasses").get<int>();
      double f1=0.0;
      conf_matrix = dMat::Zero(nclasses,nclasses);
      int batch_size = ad.get("batch_size").get<int>();
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	  int maxpr = std::distance(predictions.begin(),std::max_element(predictions.begin(),predictions.end()));
	  double target = bad.get("target").get<double>();
	  if (target < 0)
	    throw OutputConnectorBadParamException("negative supervised discrete target (e.g. wrong use of label_offset ?");
	  else if (target >= nclasses)
	    throw OutputConnectorBadParamException("target class has id " + std::to_string(target) + " is higher than the number of classes " + std::to_string(nclasses) + " (e.g. wrong number of classes specified with nclasses");
	  conf_matrix(maxpr,static_cast<int>(target)) += 1.0;
	}
      conf_diag = conf_matrix.diagonal();
      dMat conf_csum = conf_matrix.colwise().sum();
      dMat conf_rsum = conf_matrix.rowwise().sum();
      dMat eps = dMat::Constant(nclasses,1,1e-8);
      acc = conf_diag.sum() / conf_matrix.sum();
      precision = conf_diag.transpose().cwiseQuotient(conf_csum + eps.transpose()).sum() / static_cast<double>(nclasses);
      recall = conf_diag.cwiseQuotient(conf_rsum + eps).sum() / static_cast<double>(nclasses);
      f1 = (2.0*precision*recall) / (precision+recall);
      conf_diag = conf_diag.transpose().cwiseQuotient(conf_csum+eps.transpose()).transpose();
      for (int i=0;i<conf_matrix.cols();i++)
	conf_matrix.col(i) /= conf_csum(i);
      return f1;
    }
    
    // measure: AUC
    static double auc(const APIData &ad)
    {
      std::vector<double> pred1;
      std::vector<double> targets;
      int batch_size = ad.get("batch_size").get<int>();
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  pred1.push_back(bad.get("pred").get<std::vector<double>>().at(1));
	  targets.push_back(bad.get("target").get<double>());
	}
      return auc(pred1,targets);
    }
    static double auc(const std::vector<double> &pred, const std::vector<double> &targets)
    {
      class PredictionAndAnswer {
      public:
	PredictionAndAnswer(const float &f, const int &i)
	  :prediction(f),answer(i) {}
	~PredictionAndAnswer() {}
	float prediction;
	int answer; //this is either 0 or 1
      };
      std::vector<PredictionAndAnswer> p;
      for (size_t i=0;i<pred.size();i++)
	p.emplace_back(pred.at(i),targets.at(i));
      int count = p.size();
      
      std::sort(p.begin(),p.end(),
		[](const PredictionAndAnswer &p1, const PredictionAndAnswer &p2){return p1.prediction < p2.prediction;});

      int i,truePos,tp0,accum,tn,ones=0;
      float threshold; //predictions <= threshold are classified as zeros

      for (i=0;i<count;i++) ones+=p[i].answer;
      if (0==ones || count==ones) return 1;

      truePos=tp0=ones; accum=tn=0; threshold=p[0].prediction;
      for (i=0;i<count;i++) {
        if (p[i].prediction!=threshold) { //threshold changes
	  threshold=p[i].prediction;
	  accum+=tn*(truePos+tp0); //2* the area of trapezoid
	  tp0=truePos;
	  tn=0;
        }
        tn+= 1- p[i].answer; //x-distance between adjacent points
        truePos-= p[i].answer;            
      }
      accum+=tn*(truePos+tp0); //2* the area of trapezoid
      return accum/static_cast<double>((2*ones*(count-ones)));
    }
    
    // measure: multiclass logarithmic loss
    static double mcll(const APIData &ad)
    {
      double ll=0.0;
      int batch_size = ad.get("batch_size").get<int>();
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	  double target = bad.get("target").get<double>();
	  ll -= std::log(predictions.at(target));
	}
      return ll / static_cast<double>(batch_size);
    }

    // measure: Mathew correlation coefficient for binary classes
    static double mcc(const APIData &ad)
    {
      int nclasses = ad.get("nclasses").get<int>();
      dMat conf_matrix = dMat::Zero(nclasses,nclasses);
      int batch_size = ad.get("batch_size").get<int>();
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	  int maxpr = std::distance(predictions.begin(),std::max_element(predictions.begin(),predictions.end()));
	  double target = bad.get("target").get<double>();
	  if (target < 0)
	    throw OutputConnectorBadParamException("negative supervised discrete target (e.g. wrong use of label_offset ?");
	  else if (target >= nclasses)
	    throw OutputConnectorBadParamException("target class has id " + std::to_string(target) + " is higher than the number of classes " + std::to_string(nclasses) + " (e.g. wrong number of classes specified with nclasses");
	  conf_matrix(maxpr,static_cast<int>(target)) += 1.0;
	}
      double tp = conf_matrix(0,0);
      double tn = conf_matrix(1,1);
      double fn = conf_matrix(0,1);
      double fp = conf_matrix(1,0);
      double den = (tp+fp)*(tp+fn)*(tn+fp)*(tn+fn);
      if (den == 0.0)
	den = 1.0;
      double mcc = (tp*tn-fp*fn) / std::sqrt(den);
      return mcc;
    }
    
    static double eucll(const APIData &ad)
    {
      double eucl = 0.0;
      int batch_size = ad.get("batch_size").get<int>();
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  std::vector<double> predictions = bad.get("pred").get<std::vector<double>>();
	  std::vector<double> target;
	  if (predictions.size() > 1)
	    target = bad.get("target").get<std::vector<double>>();
	  else target.push_back(bad.get("target").get<double>());
	  for (size_t i=0;i<target.size();i++)
        if (target.at(i) >=0)
          eucl += (predictions.at(i)-target.at(i))*(predictions.at(i)-target.at(i));
	}
	return eucl / static_cast<double>(batch_size);
    }
    
    // measure: gini coefficient
    static double comp_gini(const std::vector<double> &a, const std::vector<double> &p) {
      struct K {double a, p;} k[a.size()];
      for (size_t i = 0; i != a.size(); ++i) k[i] = {a[i], p[i]};
      std::stable_sort(k, k+a.size(), [](const K &a, const K &b) {return a.p > b.p;});
      double accPopPercSum=0, accLossPercSum=0, giniSum=0, sum=0;
      for (auto &i: a) sum += i;
      for (auto &i: k) 
	{
	  accLossPercSum += i.a/sum;
	  accPopPercSum += 1.0/a.size();
	  giniSum += accLossPercSum-accPopPercSum;
	}
      return giniSum/a.size();
    }
    static double comp_gini_normalized(const std::vector<double> &a, const std::vector<double> &p) {
      return comp_gini(a, p)/comp_gini(a, a);
    }
    
    static double gini(const APIData &ad,
		       const bool &regression)
    {
      int batch_size = ad.get("batch_size").get<int>();
      std::vector<double> a(batch_size);
      std::vector<double> p(batch_size);
      for (int i=0;i<batch_size;i++)
	{
	  APIData bad = ad.getobj(std::to_string(i));
	  a.at(i) = bad.get("target").get<double>();
	  if (regression)
	    p.at(i) = bad.get("pred").get<std::vector<double>>().at(0); //XXX: could be vector for multi-dimensional regression -> TODO: in supervised mode, get best pred index ?
	  else
	    {
	      std::vector<double> allpreds = bad.get("pred").get<std::vector<double>>();
	      a.at(i) = std::distance(allpreds.begin(),std::max_element(allpreds.begin(),allpreds.end()));
	    }
	}
      return comp_gini_normalized(a,p);
    }
    
    // for debugging purposes.
    /**
     * \brief print supervised output to string
     */
    void to_str(std::string &out, const int &rmax) const
    {
      auto vit = _vcats.begin();
      while(vit!=_vcats.end())
	{
	  int count = 0;
	  out += "-------------\n";
	  out += (*vit).first + "\n";
	  auto mit = _vvcats.at((*vit).second)._cats.begin();
	  while(mit!=_vvcats.at((*vit).second)._cats.end()&&count<rmax)
	  {
	    out += "accuracy=" + std::to_string((*mit).first) + " -- cat=" + (*mit).second + "\n";
	    ++mit;
	    ++count;
	  }
	  ++vit;
	}
    }

    /**
     * \brief write supervised output object to data object
     * @param out data destination
     * @param regression whether a regression task
     * @param autoencoder whether an autoencoder architecture
     * @param has_bbox whether an object detection task
     * @param has_roi whether using feature extraction and object detection
     * @param indexed_uris list of indexed uris, if any
     */
    void to_ad(APIData &out, const bool &regression, const bool &autoencoder,
	       const bool &has_bbox, const bool &has_roi,
	       const std::unordered_set<std::string> &indexed_uris) const
    {
      static std::string cl = "classes";
      static std::string ve = "vector";
      static std::string ae = "losses";
      static std::string bb = "bbox";
      static std::string roi = "vals";
      static std::string rois = "rois";

      static std::string phead = "prob";
      static std::string chead = "cat";
      static std::string vhead = "val";
      static std::string ahead = "loss";
      static std::string last = "last";
      std::unordered_set<std::string>::const_iterator hit;
      std::vector<APIData> vpred;
      for (size_t i=0;i<_vvcats.size();i++)
	{
	  APIData adpred;
	  std::vector<APIData> v;
	  auto bit = _vvcats.at(i)._bboxes.begin();
	  auto vit = _vvcats.at(i)._vals.begin();
	  auto mit = _vvcats.at(i)._cats.begin();
	  while(mit!=_vvcats.at(i)._cats.end())
	    {
	      APIData nad;
	      if (!autoencoder)
		nad.add(chead,(*mit).second);
	      if (regression)
		nad.add(vhead,(*mit).first);
	      else if (autoencoder)
		nad.add(ahead,(*mit).first);
	      else nad.add(phead,(*mit).first);
	      if (has_bbox || has_roi)
		{
		  nad.add(bb,(*bit).second);
		  ++bit;
		}
	      if (has_roi)
		{
		  /* std::vector<std::string> keys = (*vit).second.list_keys(); */
		  /* std::copy(keys.begin(), keys.end(), std::ostream_iterator<std::string>(std::cout, "'")); */
		  /* std::cout << std::endl; */
		  nad.add(roi,(*vit).second.get("vals").get<std::vector<double>>());
		  ++vit;
		}
	      ++mit;
	      if (mit == _vvcats.at(i)._cats.end())
		nad.add(last,true);
	      v.push_back(nad);
	    }
	  if (_vvcats.at(i)._loss > 0.0) // XXX: not set by Caffe in prediction mode for now
	    adpred.add("loss",_vvcats.at(i)._loss);
	  adpred.add("uri",_vvcats.at(i)._label);
#ifdef USE_SIMSEARCH
	  if (!indexed_uris.empty() && (hit=indexed_uris.find(_vvcats.at(i)._label))!=indexed_uris.end())
	    adpred.add("indexed",true);
	  if (!_vvcats.at(i)._nns.empty() || !_vvcats.at(i)._bbox_nns.empty())
	    {
	      if (!has_roi)
		{
		  std::vector<APIData> ad_nns;
		  auto nnit = _vvcats.at(i)._nns.begin();
		  while(nnit!=_vvcats.at(i)._nns.end())
		    {
		      APIData ad_nn;
		      ad_nn.add("uri",(*nnit).second._uri);
		      ad_nn.add("dist",(*nnit).first);
		      ad_nns.push_back(ad_nn);
		      ++nnit;
		    }
		  adpred.add("nns",ad_nns);
		}
	      else // has_roi
		{
		  for (size_t bb=0;bb<_vvcats.at(i)._bbox_nns.size();bb++)
		    {
		      std::vector<APIData> ad_nns;
		      auto nnit = _vvcats.at(i)._bbox_nns.at(bb).begin();
		      while(nnit!=_vvcats.at(i)._bbox_nns.at(bb).end())
			{
			  APIData ad_nn;
			  ad_nn.add("uri",(*nnit).second._uri);
			  ad_nn.add("dist",(*nnit).first);
			  ad_nn.add("prob",(*nnit).second._prob);
			  ad_nn.add("cat",(*nnit).second._cat);
			  APIData ad_bbox;
			  ad_bbox.add("xmin",(*nnit).second._bbox.at(0));
			  ad_bbox.add("ymin",(*nnit).second._bbox.at(1));
			  ad_bbox.add("xmax",(*nnit).second._bbox.at(2));
			  ad_bbox.add("ymax",(*nnit).second._bbox.at(3));
			  ad_nn.add("bbox",ad_bbox);
			  ad_nns.push_back(ad_nn);
			  ++nnit;
			}
		      v.at(bb).add("nns",ad_nns); // v is in roi object vector
		    }
		}
	    }
#endif
	  if (regression)
	    adpred.add(ve,v);
	  else if (autoencoder)
	    adpred.add(ae,v);
	  else if (has_roi)
	    adpred.add(rois,v);
	  else adpred.add(cl,v);
	  vpred.push_back(adpred);
	}
      out.add("predictions",vpred);
    }
    
    std::unordered_map<std::string,int> _vcats; /**< batch of results, per uri. */
    std::vector<sup_result> _vvcats; /**< ordered results, per uri. */
    
    // options
    int _best = 1;
#ifdef USE_SIMSEARCH
    int _search_nn = 10; /**< default nearest neighbors per search. */
#endif
  };
  
}

#endif
