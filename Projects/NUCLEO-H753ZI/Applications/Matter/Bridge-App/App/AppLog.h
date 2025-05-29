/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef APP_LOG_H
#define APP_LOG_H

/* STM32 Logging */
#ifdef __cplusplus
extern "C" {
#endif

void stm32Log(const char * aFormat, ...);
#define STM32_LOG(...) stm32Log(__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_H */
