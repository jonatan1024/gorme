$modelname		"gorme/triangle.mdl"
$body			mybody			"triangle.smd"
$cdmaterials	""

$sequence idle triangle frame 0 0
$animation v1xmin triangle frame 3 3
$animation v1xmax triangle frame 4 4
$animation v1ymin triangle frame 2 2
$animation v1ymax triangle frame 1 1
$animation v1zmin triangle frame 5 5
$animation v1zmax triangle frame 6 6
$animation v2xmin triangle frame 9 9 
$animation v2xmax triangle frame 10 10
$animation v2ymin triangle frame 8 8
$animation v2ymax triangle frame 7 7
$animation v2zmin triangle frame 11 11
$animation v2zmax triangle frame 12 12
$sequence v1x { v1xmin v1xmax blend "v1x" -1 1 } delta autoplay hidden
$sequence v1y { v1ymin v1ymax blend "v1y" -1 1 } delta autoplay hidden
$sequence v1z { v1zmin v1zmax blend "v1z" -1 1 } delta autoplay hidden
$sequence v2x { v2xmin v2xmax blend "v2x" -1 1 } delta autoplay hidden
$sequence v2y { v2ymin v2ymax blend "v2y" -1 1 } delta autoplay hidden
$sequence v2z { v2zmin v2zmax blend "v2z" -1 1 } delta autoplay hidden
$cbox -1 -1 -1 1 1 1
$bbox -1 -1 -1 1 1 1
$hbox 0 "v0" -1 -1 -1 1 1 1 0 0 0
