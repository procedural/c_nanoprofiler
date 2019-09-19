Nanoprofiler for C / C++
------------------------

Supported architectures: Linux (x64), Windows (x64, Win32, UWP)

Usage:

```C
#include "nanoprofiler.h"

int main() {
  NanoprofilerAllocate(5000000);
  NanoprofilerBegin(0, "main");
  ...
  NanoprofilerEnd(0, "main");
  NanoprofilerOutputAndFree(0);
}
```

Redirect console output to a `.json` file and open it in a `about:tracing` tab in Chrome or Chromium browser.

`NanoprofilerEnd()` + `NanoprofilerBegin()` overhead for `-O0` debug build: 330 nanoseconds, 165 nanoseconds for each call (measured on 2.2 GHz 2 core AMD laptop, Ubuntu 18.04.1, Clang 7.0).

<img width="1280px" src="https://raw.githubusercontent.com/procedural/c_nanoprofiler/master/NanoprofilerEndBeginOverhead.png" />

FAQ:
---

 * Q: My timeline is drawn incorrectly, some of the labels are missing.
 * A: This can be caused by calling `NanoprofilerBegin` without `NanoprofilerEnd` and vice versa.
