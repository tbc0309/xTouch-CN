# Third Party LICENSES

The native firmware links ESP-IDF, standalone ArduinoJson, LVGL, JPEGDEC and optional PNGdec/Espressif components. It does not link the Arduino framework, PubSubClient, TFT_eSPI, DallasTemperature or the Arduino OneWire library. Historical notices below are retained for original/derived code and assets; they are not a current dependency list. Exact build versions are pinned in `boards/dependencies.json` and component manifests.

## Image decoders

PNGdec and JPEGDEC are Copyright 2020 BitBank Software, Inc., Apache-2.0. The full PNGdec notice is retained in [lib/PNGdec/LICENSE](lib/PNGdec/LICENSE). JPEGDEC is pinned to [86282979224c8a32fd51e091ed5a35b0c699a52b](https://github.com/bitbank2/JPEGDEC/blob/86282979224c8a32fd51e091ed5a35b0c699a52b/LICENSE); its full notice is fetched with that source dependency.

## ArduinoJson

```
Copyright © 2014-2023, Benoit BLANCHON

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## DallasTemperature

```
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
```

## LVGL

```
MIT licence
Copyright (c) 2021 LVGL Kft

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## OneWire

```
Copyright (c) 2007, Jim Studt (original old version - many contributors since)

The latest version of this library may be found at:
http://www.pjrc.com/teensy/td_libs_OneWire.html

OneWire has been maintained by Paul Stoffregen (paul@pjrc.com) since
January 2010.

DO NOT EMAIL for technical support, especially not for ESP chips!
All project support questions must be posted on public forums
relevant to the board or chips used. If using Arduino, post on
Arduino's forum. If using ESP, post on the ESP community forums.
There is ABSOLUTELY NO TECH SUPPORT BY PRIVATE EMAIL!

Github's issue tracker for OneWire should be used only to report
specific bugs. DO NOT request project support via Github. All
project and tech support questions must be posted on forums, not
github issues. If you experience a problem and you are not
absolutely sure it's an issue with the library, ask on a forum
first. Only use github to report issues after experts have
confirmed the issue is with OneWire rather than your project.

Back in 2010, OneWire was in need of many bug fixes, but had
been abandoned the original author (Jim Studt). None of the known
contributors were interested in maintaining OneWire. Paul typically
works on OneWire every 6 to 12 months. Patches usually wait that
long. If anyone is interested in more actively maintaining OneWire,
please contact Paul (this is pretty much the only reason to use
private email about OneWire).

OneWire is now very mature code. No changes other than adding
definitions for newer hardware support are anticipated.

Version 2.3:
Unknown chip fallback mode, Roger Clark
Teensy-LC compatibility, Paul Stoffregen
Search bug fix, Love Nystrom

Version 2.2:
Teensy 3.0 compatibility, Paul Stoffregen, paul@pjrc.com
Arduino Due compatibility, http://arduino.cc/forum/index.php?topic=141030
Fix DS18B20 example negative temperature
Fix DS18B20 example's low res modes, Ken Butcher
Improve reset timing, Mark Tillotson
Add const qualifiers, Bertrik Sikken
Add initial value input to crc16, Bertrik Sikken
Add target_search() function, Scott Roberts

Version 2.1:
Arduino 1.0 compatibility, Paul Stoffregen
Improve temperature example, Paul Stoffregen
DS250x_PROM example, Guillermo Lovato
PIC32 (chipKit) compatibility, Jason Dangel, dangel.jason AT gmail.com
Improvements from Glenn Trewitt:

- crc16() now works
- check_crc16() does all of calculation/checking work.
- Added read_bytes() and write_bytes(), to reduce tedious loops.
- Added ds2408 example.
  Delete very old, out-of-date readme file (info is here)

Version 2.0: Modifications by Paul Stoffregen, January 2010:
http://www.pjrc.com/teensy/td_libs_OneWire.html
Search fix from Robin James
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27
Use direct optimized I/O in all cases
Disable interrupts during timing critical sections
(this solves many random communication errors)
Disable interrupts during read-modify-write I/O
Reduce RAM consumption by eliminating unnecessary
variables and trimming many to 8 bits
Optimize both crc8 - table version moved to flash

Modified to work with larger numbers of devices - avoids loop.
Tested in Arduino 11 alpha with 12 sensors.
26 Sept 2008 -- Robin James
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27

Updated to work with arduino-0008 and to include skip() as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM. Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Jim Studt's original library was modified by Josh Larios.

Tom Pollard, pollard@alum.mit.edu, contributed around May 20, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains. In any event that was..
(copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
```

## PubSubClient

```
Copyright (c) 2008-2020 Nicholas O'Leary

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## TFT_eSPI

```
Software License Agreement (FreeBSD License)

Copyright (c) 2023 Bodmer (https://github.com/Bodmer)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
```

Adafruit_GFX ORIGINAL LIBRARY LICENSE:

```
Software License Agreement (BSD License)

Copyright (c) 2012 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
```

## XPT2046_Touchscreen

```
/* Touchscreen library for XPT2046 Touch Controller Chip
 * Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
```

## GitHub Octicons

The settings footer uses the `mark-github-16` silhouette from [primer/octicons](https://github.com/primer/octicons), rasterized as a 16px alpha mask.

```
MIT License

Copyright (c) 2026 GitHub Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

# Optional board audio

S3/P4 optional sound support uses Espressif `esp_codec_dev` 1.5.9 (Apache-2.0), selected for P4's ES8311 codec. The dependency manager retains the component's license. S3's PCM5101 output uses the ESP-IDF I2S driver. No third-party sound recordings are included; short tones are synthesized locally.

# Web installer

The optional static website in `web/` uses [ESP Web Tools](https://github.com/esphome/esp-web-tools), licensed under Apache-2.0. It is bundled from the pinned npm package during website deployment. Its license is copied into the generated site's `licenses/esp-web-tools/` directory; bundled dependency notices are retained in generated JavaScript. esbuild is a build-time dependency licensed under MIT. ESP Web Tools runs in the browser. esbuild also minifies the firmware management portal during builds.

The P4 revision-3+ installer includes Espressif [esp-flasher-stub v1.4.1](https://github.com/espressif/esp-flasher-stub/releases/tag/v1.4.1), dual licensed under Apache-2.0 OR MIT. Its licenses and provenance are in `web/flasher/` and copied into the website's `licenses/esp-flasher-stub/` directory. Targeted build patches retain the pinned esptool-js API while updating P4 detection and revision handling.

Browser installation uses [SparkMD5 3.0.2](https://github.com/satazor/js-spark-md5) to compare written segments with device checksums. Its alternative MIT license is included in `web/flasher/LICENSE.spark-md5` and in the generated website.

P4 register setup and revision decoding follow Espressif esptool 5.6.0 (GPL-2.0-or-later). Attribution and the upstream license are retained in `web/flasher/NOTICE.md` and `LICENSE.esptool`; the adaptation follows this project's GPLv3 license.

## OPPO Sans 4.0

OPPO Sans（含中文及西文，3 款字重）允许个人或企业免费使用，含商业用途，版权归 OPPO 广东移动通信有限公司所有。

OPPO Sans (Chinese and Western characters, three weights) is free for personal and business use, including commercial use. Copyright belongs to OPPO Guangdong Mobile Communications Co., Ltd.

The firmware uses an OPPO Sans 4.0 subset at weight 600. Japanese kana and Korean characters absent from the supplied font use Noto Sans CJK fallback glyphs. The original TTF is not included. The free-use notice does not relicense the fonts under the project software license.

Official notice: [OPPO / ColorOS](https://www.coloros.com/article/A00000050/). Supplementary font licenses: [Noto Sans CJK](docs/licenses/NotoSansCJK-OFL.txt).

## ES3C35P panel initialization

The ST77922 hardware command table is adapted from [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32/tree/38bc4f443b96da306a4f7c77375042e46d588e64/main/boards/lcdwiki-es3c35p), pinned to commit `38bc4f443b96da306a4f7c77375042e46d588e64`. Its full MIT notice is retained in [LICENSE.hardware-init](boards/lcdwiki-es3c35p/LICENSE.hardware-init). This attribution covers the hardware initialization table, not a dependency on the reference application.

The native ES3C35P port additionally uses Espressif esp_lcd_st77922 2.1.1, esp_codec_dev 1.5.9 and led_strip 3.1.1. Their upstream licenses remain in the IDF-managed components; exact versions are pinned in the board component manifest.

## Additional test-board components

The BOX-3, Korvo-2 and Tab5 board support is pinned to Espressif ESP-BSP revision `f259a6abd5808019b1903a422092a4234ca95a51` (Apache-2.0). The retained Tab5 display, touch, audio and power sources preserve their copyright notices; provenance and local changes are recorded in [SOURCE.json](boards/candidates/common/xtouch_tab5_hardware/SOURCE.json), with the full [Apache-2.0 license](boards/candidates/common/xtouch_tab5_hardware/LICENSE). Optional capture, USB, sensor and storage implementations are not included in that retained component.

Other board-specific IDF components retain their original licenses through the dependency manager. Exact component versions are locked in the corresponding project `dependencies.lock` files. Board product images identify hardware variants; their source pages are recorded in [the image source inventory](docs/images/test-boards/SOURCES.json).
