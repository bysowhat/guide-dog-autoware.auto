// Copyright 2021 The Autoware Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef POLYGON_REMOVER__VISIBILITY_CONTROL_HPP_
#define POLYGON_REMOVER__VISIBILITY_CONTROL_HPP_

////////////////////////////////////////////////////////////////////////////////
#if defined(__WIN32)
  #if defined(POLYGON_REMOVER_BUILDING_DLL) || defined(POLYGON_REMOVER_EXPORTS)
    #define POLYGON_REMOVER_PUBLIC __declspec(dllexport)
    #define POLYGON_REMOVER_LOCAL
  #else  // defined(POLYGON_REMOVER_BUILDING_DLL) || defined(POLYGON_REMOVER_EXPORTS)
    #define POLYGON_REMOVER_PUBLIC __declspec(dllimport)
    #define POLYGON_REMOVER_LOCAL
  #endif  // defined(POLYGON_REMOVER_BUILDING_DLL) || defined(POLYGON_REMOVER_EXPORTS)
#elif defined(__linux__)
  #define POLYGON_REMOVER_PUBLIC __attribute__((visibility("default")))
  #define POLYGON_REMOVER_LOCAL __attribute__((visibility("hidden")))
#elif defined(__APPLE__)
  #define POLYGON_REMOVER_PUBLIC __attribute__((visibility("default")))
  #define POLYGON_REMOVER_LOCAL __attribute__((visibility("hidden")))
#else
  #error "Unsupported Build Configuration"
#endif

#endif  // POLYGON_REMOVER__VISIBILITY_CONTROL_HPP_
