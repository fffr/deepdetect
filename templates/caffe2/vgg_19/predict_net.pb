
VGG_ILSVRC_19_layersL
data
	conv1_1_w
	conv1_1_bconv1_1"Conv*

stride*
pad*

kernel
conv1_1conv1_1"ReluO
conv1_1
	conv1_2_w
	conv1_2_bconv1_2"Conv*

stride*
pad*

kernel
conv1_2conv1_2"ReluY
conv1_2pool1"MaxPool*

stride*
pad *

kernel*
order"NCHW*

legacy_padM
pool1
	conv2_1_w
	conv2_1_bconv2_1"Conv*

stride*
pad*

kernel
conv2_1conv2_1"ReluO
conv2_1
	conv2_2_w
	conv2_2_bconv2_2"Conv*

stride*
pad*

kernel
conv2_2conv2_2"ReluY
conv2_2pool2"MaxPool*

stride*
pad *

kernel*
order"NCHW*

legacy_padM
pool2
	conv3_1_w
	conv3_1_bconv3_1"Conv*

stride*
pad*

kernel
conv3_1conv3_1"ReluO
conv3_1
	conv3_2_w
	conv3_2_bconv3_2"Conv*

stride*
pad*

kernel
conv3_2conv3_2"ReluO
conv3_2
	conv3_3_w
	conv3_3_bconv3_3"Conv*

stride*
pad*

kernel
conv3_3conv3_3"ReluO
conv3_3
	conv3_4_w
	conv3_4_bconv3_4"Conv*

stride*
pad*

kernel
conv3_4conv3_4"ReluY
conv3_4pool3"MaxPool*

stride*
pad *

kernel*
order"NCHW*

legacy_padM
pool3
	conv4_1_w
	conv4_1_bconv4_1"Conv*

stride*
pad*

kernel
conv4_1conv4_1"ReluO
conv4_1
	conv4_2_w
	conv4_2_bconv4_2"Conv*

stride*
pad*

kernel
conv4_2conv4_2"ReluO
conv4_2
	conv4_3_w
	conv4_3_bconv4_3"Conv*

stride*
pad*

kernel
conv4_3conv4_3"ReluO
conv4_3
	conv4_4_w
	conv4_4_bconv4_4"Conv*

stride*
pad*

kernel
conv4_4conv4_4"ReluY
conv4_4pool4"MaxPool*

stride*
pad *

kernel*
order"NCHW*

legacy_padM
pool4
	conv5_1_w
	conv5_1_bconv5_1"Conv*

stride*
pad*

kernel
conv5_1conv5_1"ReluO
conv5_1
	conv5_2_w
	conv5_2_bconv5_2"Conv*

stride*
pad*

kernel
conv5_2conv5_2"ReluO
conv5_2
	conv5_3_w
	conv5_3_bconv5_3"Conv*

stride*
pad*

kernel
conv5_3conv5_3"ReluO
conv5_3
	conv5_4_w
	conv5_4_bconv5_4"Conv*

stride*
pad*

kernel
conv5_4conv5_4"ReluY
conv5_4pool5"MaxPool*

stride*
pad *

kernel*
order"NCHW*

legacy_pad
pool5
fc6_w
fc6_bfc6"FC
fc6fc6"Relu9
fc6fc6	_fc6_mask"Dropout*
ratio   ?*
is_test
fc6
fc7_w
fc7_bfc7"FC
fc7fc7"Relu9
fc7fc7	_fc7_mask"Dropout*
ratio   ?*
is_test
fc7
fc8_w
fc8_bfc8"FC
fc8prob"Softmax:data:	conv1_1_w:	conv1_1_b:	conv1_2_w:	conv1_2_b:	conv2_1_w:	conv2_1_b:	conv2_2_w:	conv2_2_b:	conv3_1_w:	conv3_1_b:	conv3_2_w:	conv3_2_b:	conv3_3_w:	conv3_3_b:	conv3_4_w:	conv3_4_b:	conv4_1_w:	conv4_1_b:	conv4_2_w:	conv4_2_b:	conv4_3_w:	conv4_3_b:	conv4_4_w:	conv4_4_b:	conv5_1_w:	conv5_1_b:	conv5_2_w:	conv5_2_b:	conv5_3_w:	conv5_3_b:	conv5_4_w:	conv5_4_b:fc6_w:fc6_b:fc7_w:fc7_b:fc8_w:fc8_bBprob