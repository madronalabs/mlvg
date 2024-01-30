
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "madronalib.h"
#include "MLMath2D.h"


#if ML_MAC
  #include "native/MLFilesMac.mm"
#elif ML_WINDOWS
  #include "native/MLFilesWin.cpp"
#endif
