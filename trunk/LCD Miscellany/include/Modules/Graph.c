struct Graph {
	var %data, %type, %points, %lastUpdate, %min, %max, %sides;

	function Graph($_type, $_min, $_max, $_points, $_sides) {
		%type = $_type;
		%points = $_points;
		if (!$_points) $_points = 240;
		%data = list();
		%data[$_points-1] = 0;
		%min = $_min;
		%max = $_max;
		%sides = $_sides;
	}

	function Update($val) {
		pop(%data, 0);
		if ($val < %min) $val = %min;
		if ($val > %max) $val = %max;
		push_back(%data, $val);
	}

	function Draw($x1, $y1, $x2, $y2) {
		$dx = ($x1 - $x2) / abs($x1 - $x2);

		if (%sides & 2) {
			DrawRect($x1, $y2, $x2, $y2);
			$y2++;
		}
		if (%sides & 1) {
			DrawRect($x1, $y1, $x1, $y2);
			DrawRect($x2, $y1, $x2, $y2);
			$x1 -= $dx;
			$x2 += $dx;
		}

		$range = ($y2-$y1)*1.0/(%max-%min);

		// For rounding.  Rounding is done by removing decimal, I believe.
		$y15 = $y1 + 0.5;

		$x2 -= $dx;

		$v = %points;
		while ($x1 != $x2) {
			$v--;
			$x2 += $dx;
			$p = (%data[$v]-%min) * $range + $y15;
			DrawRect($x2, $p, $x2, $y1);
		}
	}
};

function DrawMultiGraph($graphs, $x1, $y1, $x2, $y2, $bgColor, $padding) {
	$colors = list(RGB(0,224,0), RGB(255,0,0), RGB(0,255,255), RGB(255,255,0), RGB(255,0,255), RGB(0,0,0));
	$dx = ($x1 - $x2) / abs($x1 - $x2);
	if (!$bgColor) $bgColor = RGB(0,0,96);
	ColorRect($x1, $y1, $x2, $y2, $bgColor);

	// For rounding.  Rounding is done by removing decimal, I believe.
	$y15 = $y1 + 0.5;

	if ($y1 < $y2) {
		$y1 += $padding[1];
		$y2 -= $padding[3];
	}
	else {
		$y2 += $padding[1];
		$y1 -= $padding[3];
	}
	if ($dx < 0) {
		$x1 += $padding[0];
		$x2 -= $padding[2];
	}
	else {
		$x2 += $padding[0];
		$x1 -= $padding[2];
	}

	$origDrawColor = SetDrawColor();
	for ($i=0; $i<size($graphs); $i++) {
		SetDrawColor($colors[$i % size($colors)]);
		$graph = $graphs[$i];
		$min = $graph.min;
		$max = $graph.max;
		$v = $graph.points-1;
		$data = $graph.data;
		$range = ($y2-$y1)*1.0/($max-$min);

		$p = ($data[$v]-$min) * $range + $y15;
		$x2old = $x2;

		while ($x1 != $x2) {
			$v--;
			DrawLine($x2, $p, $x2 += $dx, $p = ($data[$v]-$min) * $range + $y15);
		}

		$x2 = $x2old;
	}
	SetDrawColor($origDrawColor);
};
