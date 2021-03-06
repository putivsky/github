/*
 * The Software License
 * =================================================================================
 * Copyright (c) 2003-2009 The Terimber Corporation. All rights reserved.
 * =================================================================================
 * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution.
 * The end-user documentation included with the redistribution, if any, 
 * must include the following acknowledgment:
 * "This product includes software developed by the Terimber Corporation."
 * =================================================================================
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
 * IN NO EVENT SHALL THE TERIMBER CORPORATION OR ITS CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ================================================================================
*/

#ifndef _terimber_ostypes_h_
#define _terimber_ostypes_h_

// base types
typedef signed char			sb1_t;
typedef unsigned char		ub1_t;

#if defined(_MSC_VER) && (_MSC_VER > 1200) 

typedef __int16				sb2_t;
typedef unsigned __int16	ub2_t;
typedef __int32				sb4_t;
typedef unsigned __int32	ub4_t;
typedef	__int64				sb8_t;
typedef unsigned __int64	ub8_t;

#if defined(_MSC_VER) && (_MSC_VER < 1400) 
#ifndef wchar_t
typedef unsigned short		wchar_t;
#endif
#endif

#else

typedef short				sb2_t;
typedef unsigned short		ub2_t;
typedef long				sb4_t;
typedef unsigned long		ub4_t;
typedef	__int64				sb8_t;
typedef unsigned __int64	ub8_t;

#ifndef wchar_t
typedef unsigned short		wchar_t;
#endif

#endif


typedef struct _terimber_guid_
{
	ub4_t Data1;
	ub2_t Data2;
	ub2_t Data3;
	ub1_t Data4[8];
} guid_t;

const guid_t null_uuid = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

// guid generator
guid_t uuid_gen();

#endif // _terimber_ostypes_h_