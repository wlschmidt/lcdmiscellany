#import <Views\View.c>
#requires <Modules\FileBrowser.c>
#requires <Modules\TextEditor.c>
#requires <Framework\Theme.c>
#requires <util\G15.c>

struct FileTextEdit {
	var %editor,
		%path,
		%file,
		%error;
	function FileTextEdit($_path, $_file, $_width, $_height, $_font) {
		%path = $_path;
		%file = $_file;
		%editor = TextEditor($_width, $_height, $_font);
		if (!%editor.Load($_file)) {
			%error = 1;
		}
	}
};

struct TextEditorView extends View {
	var %browser,
		%browserPath,
		%files,
		%current,
		%blinkTimer,
		%lastPath,
		%displayFileName,
		%displayFileCounter;

	function TextEditorView () {
		%InitFonts();
		%InitImages();

		%noDrawOnCounterUpdate = 1;
		%noDrawOnAudioChange = 1;
		%current = 0;
		%files = list();
		%SetFocus();
	}

	function %MakeLoadBrowser() {
		if (IsNull(%browser)) {
			%browser = FileBrowser(, 1, "Text Editor");
		}
	}

	function UnfocusAll() {
		%browser.Unfocus();
		for ($i=0; $i<size(%files); $i++) {
			%files[$i].editor.Unfocus();
		}
	}

	function SetFocus() {
		if (%current == size(%files)) {
			%MakeLoadBrowser();
		}
		%UnfocusAll();
		if (%hasFocus) {
			if (%current == size(%files)) {
				%browser.Focus();
			}
			else {
				%files[%current].editor.Focus();
			}
		}
	}

	function CounterUpdate() {
		if (%displayFileCounter) {
			%displayFileCounter --;
			return;
		}
		%displayFileName = null;
	}

	function NameOverlay() {
		if (size(%displayFileName)) {
			%displayFileName = null;
			NeedRedraw();
		}

		if (%current < size(%files)) {
			%displayFileCounter = 2;
			%displayFileName = FormatText(substring(%files[%current].file, size(%files[%current].path), -1), 150);
			NeedRedraw();
		}
	}

	function KeyDown($event, $param, $modifiers, $vk) {
		if ($vk == VK_ESCAPE) {
			if (%current == size(%files)) {
				if (%browser.KeyDown(@$)) return 1;
			}
			%Unfocus();
			NeedRedraw();
			return 1;
		}
		if (size(%displayFileName)) {
			%displayFileName = null;
			NeedRedraw();
			return 1;
		}
		if (%current != size(%files)) {
			if (($modifiers & 2) && !($modifiers & 4)) {
				if ($vk == VK_S) {
					%SaveFile();
				}
				else if ($vk == VK_W) {
					if (%current < size(%files)) {
						%UnfocusAll();
						pop(%files, %current);
						if (%current == size(%files))
							%current = 0;
						%SetFocus();
					}
				}
				else {
					$notMunched = 1;
				}
				if (!$notMunched) {
					NeedRedraw();
					return 1;
				}
			}
			return %files[%current].editor.KeyDown(@$);
		}
		else {
			$res = %browser.KeyDown(@$);
			if ($vk == VK_RETURN) {
				if (IsList($res)) {
					%LoadFile(@$res);
				}
			}
			return 1;
		}
	}

	function LoadFile($path, $file) {
		%lastPath = $path;

		$res = GetMaxRes();
		$font = GetThemeFont(%fontIds[IsHighRes(@$res)]);

		$file = FileTextEdit($path, $file, $res[0], $res[1], $font);
		if (!$file.error) {
			%current = size(%files);
			%files[%current] = $file;
			%SetFocus();
		}
		NeedRedraw();
	}

	function SaveFile() {
		%files[%current].editor.Save(%files[%current].file);
	}

	function G15ButtonDown($event, $param, $buttons) {
		if ($buttons & 0x3F) {
			if (%current == size(%files) && %browser.G15ButtonDown(@$)) {
				return 1;
			}

			$state = G15GetButtonsState();
			if ($buttons & 3) {
				if (!($state & ($buttons^3)) && size(%files)) {
					$dir = 1;
					if ($buttons == 1) $dir = -1;

					%UnfocusAll();

					%current = (%current + size(%files)+1+$dir) % (size(%files)+1);

					%SetFocus();
					%NameOverlay();
				}
			}
			else {
				if (!($state & ($buttons^12))) {
					if ($buttons & 8) {
						%Unfocus();
					}
					else {
						if (%current != size(%files)) {
						}
						else {
							%KeyDown(,,,VK_RETURN);
						}
					}
				}
			}
			NeedRedraw();
			return 1;
		}
	}

	function Focus() {
		if (!%hasFocus) {
			RegisterKeyEvent(3<<16, VK_ESCAPE);
			%NameOverlay();
			%hasFocus = 1;
			%SetFocus();
		}
	}

	function Unfocus() {
		if (%hasFocus) {
			UnregisterKeyEvent(3<<16, VK_ESCAPE);
			%hasFocus = 0;
			%SetFocus();
		}
	}

	function Draw($event, $param, $name, $res) {
		ClearScreen();

		if (%current == size(%files)) {
			%browser.Draw(@$);
		}
		else {
			$highRes = IsHighRes(@$res);

			$font = GetThemeFont(%fontIds[$highRes]);

			UseFont($font);
			%files[%current].editor.ChangeFormat($res[0], $res[1], $font);
			%files[%current].editor.Draw(0, 0, $res);

			if (size(%displayFileName)) {
				$box = TextSize(%displayFileName);
				$width = $box[0];
				$height = $box[1];
				$left = 80-$width/2-4;
				$top = 21-$height/2-1;
				$right = 80+$width/2+4;
				$bottom = 21+$height/2+1;
				DoubleBox($left, $top, $right, $bottom);
				DisplayText(%displayFileName, 80-($width-1)/2, $top+2);
			}
		}
	}
};
