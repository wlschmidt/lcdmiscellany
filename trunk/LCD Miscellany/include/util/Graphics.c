function DoubleBox($left, $top, $right, $bottom) {
	DrawRect(@$);
	ClearRect($left+1, $top+1, $right-1, $bottom-1);
}

