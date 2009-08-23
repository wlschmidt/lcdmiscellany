#import <constants.h>
#requires <util\Text.c>
#requires <util\G15.c>
#requires <Modules\LineEditor.c>
#requires <framework\Overlay.c>

struct FileBrowser {
	var %path, %sel, %hasFocus, %stealArrows, %fontIds, %selInfo, %pathName;

	var %run, %runHistory, %runHistoryItem;
	var %rename, %renameName, %pageHeight;

	var %search, %searchTime;
	function FileBrowser($_path, $_stealArrows, $_pathName) {
		%pathName = $_pathName;
		%path = $_path;
		if (size(%pathName)) {
			%path = GetString("File Browser", %pathName, $_path);
			$file = GetString("File Browser", %pathName +s " file");
		}
		if (size(%path) == 0 || %path[1] != ':' || %path[size(%path)-1] != "\") %path = "C:\";
		else if (size($file)) {
			$files = FileInfo(%path +s "*");
			for ($i=0; $i<size($files); $i++) {
				if ($files[$i].name ==s $file) {
					%sel = $i;
					break;
				}
			}
		}
		%stealArrows = $_stealArrows;
		%fontIds = list(RegisterThemeFont("smallFileBrowserFont"), RegisterThemeFont("bigFileBrowserFont"));
	}

	function Focus() {
		if (!%hasFocus) {
			RegisterKeyEvent(0, VK_VOLUME_DOWN, VK_VOLUME_UP);
			if (%stealArrows) {
				RegisterKeyRange(3<<16, VK_0, VK_Z);
				RegisterKeyEvent(3<<16, VK_BACK, VK_TAB);
				RegisterKeyRange(3<<16, VK_SPACE, VK_HELP);
				RegisterKeyRange(3<<16, VK_NUMPAD0, VK_DIVIDE);
				RegisterKeyRange(3<<16, 0xBA, 0xC0);
				RegisterKeyRange(3<<16, 0xDB, 0xDF);
				RegisterKeyRange(3<<16, 0xE1, 0xE4);
				RegisterKeyEvent(3<<16, VK_RETURN, VK_ESCAPE);
				RegisterKeyRange(3<<16, VK_F1, VK_F12);
			}
			%hasFocus = 1;
		}
	}

	function Unfocus() {
		%run = null;
		%rename = null;
		%search = "";
		if (%hasFocus) {
			UnregisterKeyEvent(0, VK_VOLUME_DOWN, VK_VOLUME_UP);
			%hasFocus = 0;
			if (%stealArrows) {
				UnregisterKeyRange(3<<16, VK_0, VK_Z);
				UnregisterKeyEvent(3<<16, VK_BACK, VK_TAB);
				UnregisterKeyRange(3<<16, VK_SPACE, VK_HELP);
				UnregisterKeyRange(3<<16, VK_NUMPAD0, VK_DIVIDE);
				UnregisterKeyRange(3<<16, 0xBA, 0xC0);
				UnregisterKeyRange(3<<16, 0xDB, 0xDF);
				UnregisterKeyRange(3<<16, 0xE1, 0xE4);
				UnregisterKeyEvent(3<<16, VK_RETURN, VK_ESCAPE);
				UnregisterKeyRange(3<<16, VK_F1, VK_F12);
			}
		}
	}

	function KeyDown($event, $param, $modifiers, $vk, $string) {
		$vk = MediaFlip($vk);
		NeedRedraw();
		if ($vk == VK_ESCAPE) {
			// Takes care of rename and run dialogs.
			if (%G15ButtonDown(, , G15_CANCEL)) {
				return 1;
			}
			return 0;
		}
		if (!IsNull(%rename)) {
			if ($vk == VK_RETURN) {
				$text = %rename.GetText();
				$oldName = %renameName;
				%G15ButtonDown(, , G15_CANCEL);
				if (size($text)) {
					if (MoveFile(%path +s $oldName, %path +s $text)) {
						KeyDown(, , , $vk, $string);
					}
				}
			}
			else
				%rename.KeyDown(@$);
			return 1;
		}
		if (!IsNull(%run)) {
			if ($vk == VK_RETURN) {
				// Have to do this first.  Run can act like a wait function, unfortunately,
				// as windows messages can be processed in the mean time.
				// MSDN does not seem to mention this.  :(
				$text = %run.GetText();
				%G15ButtonDown(, , G15_CANCEL);
				if (size($text)) {
					$temp = RegExp($text, "^\w*|"([^|"]*)|"\w*(.*)$");//"
					if (!size($temp[0][0])) {
						$temp = RegExp($text, "^\w*([^\w]*)\w*(.*)$");
					}
					if (size($temp[0][0])) {
						if (!Run($temp[0][0], $temp[1][0], 2))
							RunSimple($temp[0][0], $temp[1][0], 2);
						%runHistory = GetString("File Browser", "Run History", "le");
						if (IsString(%runHistory) && IsList(%runHistory = Bedecode(%runHistory))) {
							%runHistory[size(%runHistory)] = $text;
							if (size(%runHistory) > 50) {
								pop_range(%runHistory,0,0);
							}
							SaveString("File Browser", "Run History", BencodeExact(%runHistory), 1);
						}
					}
				}
			}
			else if ($vk == VK_UP) {
				if (%runHistoryItem) {
					%run.SetText(%runHistory[--%runHistoryItem]);
				}
			}
			else if ($vk == VK_DOWN) {
				if (%runHistoryItem < size(%runHistory)-1) {
					%run.SetText(%runHistory[++%runHistoryItem]);
				}
				else {
					$text = %run.GetText();
					if (size($text)) {
						%runHistory[%runHistoryItem++] = $text;
						%run.SetText("");
					}
				}
			}
			else
				%run.KeyDown(@$);
			return 1;
		}
		if (($modifiers & 2) || $vk == VK_F2) {
			if ($vk == VK_V) {
				// Not the right way to do it.  Can't find out how without menus and InvokeCommand,
				// which would pop up a dialog on overwrite, which I don't want.
				/*
				$files = GetClipboardData(CF_HDROP);
				if ($files[0] == CF_HDROP) {
					$files = strsplit($files, "|r|n");
					for ($i=0; $i<size($files); $i++) {
					}
				}
				//*/
			}
			else if (!IsString(%selInfo) && %selInfo.name !=s "..") {
				if ($vk == VK_R) {
					%runHistory = GetString("File Browser", "Run History", "le");
					if (!IsString(%runHistory) || !IsList(%runHistory = Bedecode(%runHistory))) {
						%runHistory = null;
					}
					%runHistoryItem = size(%runHistory);

					%run = LineEditor(144, 0);
					%run.Focus();
					$text = %path +s %selInfo.name;
					if (strstr($text, " ")) $text = "|"" +s $text +s "|"";
					%run.SetText($text);
				}
				else if ($vk == VK_F2) {
					%rename = LineEditor(160, 0);
					%rename.SetText(%selInfo.name);
					%rename.Focus();
					%renameName = %selInfo.name;
				}
			}
			return 1;
		}
		if ($vk == VK_DELETE || $vk == VK_BACK) {
			%search = "";
			MessageBoxOverlay(eventHandler, "Delete File:|n" +S %selInfo.name +S "?",
								"Yes", "No", "DeleteFile",,,,list(%path +s %selInfo.name));
			return 1;
		}
		if (size($string) && $vk != VK_RETURN) {
			if (GetTickCount() - %searchTime >= 1400) {
				%search = "";
			}
			%searchTime = GetTickCount();
			%search +=s $string;
			$files = list(@FileInfo(%path +s "*"), @DriveList());
			$index = %sel;

			while (1) {
				if (IsString($files[$index])) {
					$name = $files[$index];
				}
				else {
					$name = $files[$index].name;
				}
				if (substring($name, 0, size(%search)) ==s %search) break;
				$index = ($index + 1) % size($files);
				if ($index == %sel) break;
			}
			%sel = $index;
			return 1;
		}
		%search = "";
		if ($vk == VK_VOLUME_UP || $vk == VK_DOWN) {
			%sel++;
		}
		else if ($vk == VK_VOLUME_DOWN || $vk == VK_UP) {
			%sel--;
		}
		else if ($vk == VK_NEXT || $vk == VK_RIGHT) {
			$count = size(FileInfo(%path +s "*")) + size(DriveList());
			%sel += %pageHeight;
			if (%sel >= $count) %sel = $count - 1;
		}
		else if ($vk == VK_PRIOR || $vk == VK_LEFT) {
			if (%sel < %pageHeight) {
				%sel = 0;
			}
			else {
				%sel -= %pageHeight;
			}
		}
		else if ($vk == VK_HOME) {
			%sel = 0;
		}
		else if ($vk == VK_END) {
			%sel = -1;
		}
		else if (($vk == VK_RETURN) && !IsNull(%selInfo)) {
			if (IsString(%selInfo)) {
				%sel = 0;
				%path = %selInfo;
			}
			else if (%selInfo.attributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (%selInfo.name !=s "..") {
					%sel = 0;
					%path = %path +s %selInfo.name +s "\"; //"
				}
				else {
					$match = RegExp(%path, "^(.*\\)(.*)\\$");
					$newPath = $match[0][0];
					$oldDir = $match[1][0];
					if (IsString($newPath)) {
						%path = $newPath;
						$files = FileInfo(%path +s "*");
						%sel = 0;
						for ($i=0; $i<size($files); $i++) {
							if ($files[$i].name ==S $oldDir) {
								%sel = $i;
								break;
							}
						}
					}
				}
			}
			else {
				if (size(%pathName)) {
					SaveString("File Browser", %pathName, %path);
					SaveString("File Browser", %pathName +s " file", %selInfo.name, 1);
				}
				return list(%path, %path +s %selInfo.name);
			}
		}
		return 1;
	}

	function G15ButtonDown($event, $param, $button, $keyboard) {
		%search = "";
		if ($button & 0x3F) {
			$button = FilterButton($button);
			if ($button == G15_UP) {
				%KeyDown(,,,VK_UP);
				return 1;
			}
			else if ($button == G15_DOWN) {
				%KeyDown(,,,VK_DOWN);
				return 1;
			}
			else if ($button & 0x3) {
				$state = GetDeviceState($keyboard);
				if ($state.type == SDK_320_240_32 || $state.type == LCD_G19) {
					if ($button == G15_LEFT) {
						%KeyDown(,,,VK_LEFT);
					}
					else if ($button == G15_RIGHT) {
						%KeyDown(,,,VK_RIGHT);
					}
				}
				else {
					if ($button == G15_LEFT) {
						%KeyDown(,,,VK_UP);
					}
					else if ($button == G15_RIGHT) {
						%KeyDown(,,,VK_DOWN);
					}
				}
				return 1;
			}
			else if ($button == G15_CANCEL) {
				if (!IsNull(%run)) {
					%run.Unfocus();
					%run = null;
					NeedRedraw();
					%runHistory = null;
					return 1;
				}
				if (!IsNull(%rename)) {
					WriteLogLn("here");
					%rename.Unfocus();
					%rename = null;
					%renameName = null;
					NeedRedraw();
					return 1;
				}
			}
		}
	}

	function Draw($event, $param, $name, $res) {
		$highRes = IsHighRes(@$res);

		$w = $res[0];
		$h = $res[1];

		$font = GetThemeFont(%fontIds[$highRes]);
		UseFont($font);

		$files = list(@FileInfo(%path +s "*"), @DriveList());

		if (!IsNull(%rename)) {
			for ($i=0; $i<size($files); $i++) {
				if (%renameName ==s $files[$i].name) {
					%sel = $i;
					break;
				}
			}
			if ($i == size($files)) {
				%G15ButtonDown(, , G15_CANCEL);
			}
			else {
				%rename.ReFormat($font, $w);
			}
		}
		if (%sel >= size($files)) {
			%sel = 0;
		}
		else if (%sel < 0) {
			%sel = size($files)-1;
		}
		$pos = 0;
		$height = GetFontHeight();
		%pageHeight = $h/$height;
		$index = %sel-(%sel % %pageHeight);
		$rightEdge = $w - 1 - 4*$highRes;
		$clearStart = $rightEdge - TextSize(" 0.00 M")[0];
		if ($highRes) {
			while ($pos < $h && $index < size($files)) {
				if ($index == %sel && %hasFocus) {
					SetDrawColor(colorHighlightText);
					SetBgColor(colorHighlightBg);
					ClearRect(0, $pos, $w, $pos+$height);
				}
				else if ($index == %sel) {
					SetDrawColor(colorHighlightUnfocusedText);
					SetBgColor(colorHighlightUnfocusedBg);
					ClearRect(0, $pos, $w, $pos+$height);
				}
				else {
					SetBgColor(colorBg);
				}
				if (!IsNull(%rename) && $index == %sel) {
					%rename.Draw(0, $pos);
				}
				else {
					if (IsString($files[$index])) {
						if ($index != %sel) SetDrawColor(colorDriveText);
						$name = $files[$index];
						if ($name[0] >s "B") {
							$info = VolumeInformation($name);
							if (size($info.volumeName)) {
								$name = $name +s "|t[" +s $info.volumeName +s "]";
							}
						}
						DisplayText($name, 1, $pos);
					}
					else if ($files[$index].attributes & FILE_ATTRIBUTE_DIRECTORY) {
						if ($index != %sel) SetDrawColor(colorDirectoryText);
						DisplayText($files[$index].name,1, $pos);
					}
					else {
						if ($index != %sel) SetDrawColor(colorText);
						DisplayText($files[$index].name,1, $pos);
						ClearRect($clearStart, $pos, $w, $pos+$height);
						DisplayTextRight(FormatSize($files[$index].bytes,,,3), $rightEdge, $pos);
					}
				}
				$pos += $height;
				$index++;
			}
		}
		else {
			while ($pos < $h && $index < size($files)) {
				if ($index == %sel && %hasFocus) {
					SetDrawColor(colorHighlightText);
					SetBgColor(colorHighlightBg);
					ClearRect(0, $pos, $w, $pos+$height);
				}
				else {
					SetDrawColor(colorText);
					SetBgColor(colorBg);
				}
				if (!IsNull(%rename) && $index == %sel) {
					%rename.Draw(0, $pos);
				}
				else {
					if (IsString($files[$index])) {
						$name = $files[$index];
						if ($name[0] >s "B") {
							$info = VolumeInformation($name);
							if (size($info.volumeName)) {
								$name = $name +s " [|2" +s $info.volumeName +s "|2]";
							}
						}
						DisplayText("|2" +s $name, 1, $pos);
					}
					else if ($files[$index].attributes & FILE_ATTRIBUTE_DIRECTORY) {
						DisplayText("|2" +s $files[$index].name,1, $pos);
					}
					else {
						DisplayText($files[$index].name,1, $pos);
						ClearRect($clearStart, $pos, $w, $pos+$height);
						DisplayTextRight(FormatSize($files[$index].bytes,,,3), $rightEdge, $pos);
					}
				}
				$pos += $height;
				$index++;
			}
		}
		SetDrawColor(colorText);
		%selInfo = $files[%sel];
		if (!IsNull(%run)) {
			%run.ReFormat($font, $w-144);
			%run.DrawBox("Run:");
		}
	}
};

