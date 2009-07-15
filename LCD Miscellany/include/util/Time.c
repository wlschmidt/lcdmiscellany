
function FormatDuration(duration) {
	$secs = $duration%60;
	$mins = ($duration/60)%60;
	$hours = $duration/3600;
	if ($secs<10) $secs = ":0" +s $secs;
	else  $secs = ":" +s $secs;
	if (!$mins) $secs = "00" +s $secs;
	else if ($mins < 10) $secs = "0" +s $mins +s $secs;
	else $secs = $mins +s $secs;
	if ($hours) $secs = $hours +s ":" +s $secs;
	return $secs;
}
