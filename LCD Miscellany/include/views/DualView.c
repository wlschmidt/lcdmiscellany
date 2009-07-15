#import <Views\View.c>

// Note:  Holds two subviews, sends focus and events to both views
// First view draws to G15 style LCDs, second to G19s.  Cannot
// be focused, designed to hold two status screens or two StatusViews.
struct DualView extends View {
	var %views;
	var %visible;

	function DualView($image) {
		if (IsString($image)) {
			%toolbarImage = LoadImage($image);
			pop($,0);
		}
		else {
			%toolbarImage = LoadImage("images\Eye.png");
		}


		RemoveNulls($);

		%views = $;
    }

    function HandleEvent($event) {
		for ($i=0; $i<size(%views); $i++) {
			if (%views[$i].call($event, $) || %views[$i].HandleEvent(@$))
				$out = 1;
		}
		return $out;
	}

	function Focus() {
		if (%visible) {
			for ($i=0; $i<size(%views); $i++) {
				%views[$i].Focus();
			}
		}
	}

	function Show() {
		if (!%visible) {
			for ($i=0; $i<size(%views); $i++) {
				%views[$i].Show();
			}
			%visible = 1;
		}
	}

	function Hide() {
		if (%visible) {
			for ($i=0; $i<size(%views); $i++) {
				%views[$i].Unfocus();
				%views[$i].Hide();
			}
			%visible = 0;
		}
	}


	function Draw($event, $param, $name, $res) {
		$width = $res[0];
		if ($width < 320) {
			%views[0].Draw(@$);
		}
		else {
			%views[1].Draw(@$);
		}
	}
};
