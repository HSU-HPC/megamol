/*
 * thecam/math/vector.cpp
 *
 * Copyright (C) 2017 TheLib Team (http://www.thelib.org/license)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of TheLib, TheLib Team, nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THELIB TEAM AS IS AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THELIB TEAM BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mmcore/thecam/math/vector.h"


#ifdef WITH_THE_XMATH
/*
 * megamol::core::thecam::math::orthonormalise
 */
void megamol::core::thecam::math::orthonormalise(vector<DirectX::XMFLOAT4>& vec1,
        vector<DirectX::XMFLOAT4>& vec2) {
    auto v1 = load_xmvector(vec1);
    auto v2 = load_xmvector(vec2);

    v1 = DirectX::XMVector3Normalize(v1);

    auto dp = DirectX::XMVector3Dot(v1, v2);
    auto p = DirectX::XMVectorMultiply(dp, v1);

    v2 = DirectX::XMVectorSubtract(v2, p);
    v2 = DirectX::XMVector3Normalize(v2);

    store_xmvector(vec1, v1);
    store_xmvector(vec2, v2);
}
#endif /* WITH_THE_XMATH */