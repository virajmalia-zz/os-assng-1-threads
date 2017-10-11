/* Copyright (C) 2001-2017 Free Software Foundation, Inc.
2	   This file is part of the GNU C Library.
3	
4	   The GNU C Library is free software; you can redistribute it and/or
5	   modify it under the terms of the GNU Lesser General Public
6	   License as published by the Free Software Foundation; either
7	   version 2.1 of the License, or (at your option) any later version.
8	
9	   The GNU C Library is distributed in the hope that it will be useful,
10	   but WITHOUT ANY WARRANTY; without even the implied warranty of
11	   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
12	   Lesser General Public License for more details.
13	
14	   You should have received a copy of the GNU Lesser General Public
15	   License along with the GNU C Library; if not, see
16	   <http://www.gnu.org/licenses/>.  */
17	
18	#ifndef _SYS_UCONTEXT_H
19	#define _SYS_UCONTEXT_H        1
20	
21	#include <features.h>
22	
23	#include <bits/types.h>
24	#include <bits/types/sigset_t.h>
25	#include <bits/types/stack_t.h>
26	
27	
28	#ifdef __USE_MISC
29	# define __ctx(fld) fld
30	#else
31	# define __ctx(fld) __ ## fld
32	#endif
33	
34	#ifdef __x86_64__
35	
36	/* Type for general register.  */
37	__extension__ typedef long long int greg_t;
38	
39	/* Number of general registers.  */
40	#define __NGREG        23
41	#ifdef __USE_MISC
42	# define NGREG        __NGREG
43	#endif
44	
45	/* Container for all general registers.  */
46	typedef greg_t gregset_t[__NGREG];
47	
48	#ifdef __USE_GNU
49	/* Number of each register in the `gregset_t' array.  */
50	enum
51	{
52	  REG_R8 = 0,
53	# define REG_R8                REG_R8
54	  REG_R9,
55	# define REG_R9                REG_R9
56	  REG_R10,
57	# define REG_R10        REG_R10
58	  REG_R11,
59	# define REG_R11        REG_R11
60	  REG_R12,
61	# define REG_R12        REG_R12
62	  REG_R13,
63	# define REG_R13        REG_R13
64	  REG_R14,
65	# define REG_R14        REG_R14
66	  REG_R15,
67	# define REG_R15        REG_R15
68	  REG_RDI,
69	# define REG_RDI        REG_RDI
70	  REG_RSI,
71	# define REG_RSI        REG_RSI
72	  REG_RBP,
73	# define REG_RBP        REG_RBP
74	  REG_RBX,
75	# define REG_RBX        REG_RBX
76	  REG_RDX,
77	# define REG_RDX        REG_RDX
78	  REG_RAX,
79	# define REG_RAX        REG_RAX
80	  REG_RCX,
81	# define REG_RCX        REG_RCX
82	  REG_RSP,
83	# define REG_RSP        REG_RSP
84	  REG_RIP,
85	# define REG_RIP        REG_RIP
86	  REG_EFL,
87	# define REG_EFL        REG_EFL
88	  REG_CSGSFS,                /* Actually short cs, gs, fs, __pad0.  */
89	# define REG_CSGSFS        REG_CSGSFS
90	  REG_ERR,
91	# define REG_ERR        REG_ERR
92	  REG_TRAPNO,
93	# define REG_TRAPNO        REG_TRAPNO
94	  REG_OLDMASK,
95	# define REG_OLDMASK        REG_OLDMASK
96	  REG_CR2
97	# define REG_CR2        REG_CR2
98	};
99	#endif
100	
101	struct _libc_fpxreg
102	{
103	  unsigned short int __ctx(significand)[4];
104	  unsigned short int __ctx(exponent);
105	  unsigned short int __glibc_reserved1[3];
106	};
107	
108	struct _libc_xmmreg
109	{
110	  __uint32_t        __ctx(element)[4];
111	};
112	
113	struct _libc_fpstate
114	{
115	  /* 64-bit FXSAVE format.  */
116	  __uint16_t                __ctx(cwd);
117	  __uint16_t                __ctx(swd);
118	  __uint16_t                __ctx(ftw);
119	  __uint16_t                __ctx(fop);
120	  __uint64_t                __ctx(rip);
121	  __uint64_t                __ctx(rdp);
122	  __uint32_t                __ctx(mxcsr);
123	  __uint32_t                __ctx(mxcr_mask);
124	  struct _libc_fpxreg        _st[8];
125	  struct _libc_xmmreg        _xmm[16];
126	  __uint32_t                __glibc_reserved1[24];
127	};
128	
129	/* Structure to describe FPU registers.  */
130	typedef struct _libc_fpstate *fpregset_t;
131	
132	/* Context to describe whole processor state.  */
133	typedef struct
134	  {
135	    gregset_t __ctx(gregs);
136	    /* Note that fpregs is a pointer.  */
137	    fpregset_t __ctx(fpregs);
138	    __extension__ unsigned long long __reserved1 [8];
139	} mcontext_t;
140	
141	/* Userlevel context.  */
142	typedef struct ucontext_t
143	  {
144	    unsigned long int __ctx(uc_flags);
145	    struct ucontext_t *uc_link;
146	    stack_t uc_stack;
147	    mcontext_t uc_mcontext;
148	    sigset_t uc_sigmask;
149	    struct _libc_fpstate __fpregs_mem;
150	  } ucontext_t;
151	
152	#else /* !__x86_64__ */
153	
154	/* Type for general register.  */
155	typedef int greg_t;
156	
157	/* Number of general registers.  */
158	#define __NGREG        19
159	#ifdef __USE_MISC
160	# define NGREG        __NGREG
161	#endif
162	
163	/* Container for all general registers.  */
164	typedef greg_t gregset_t[__NGREG];
165	
166	#ifdef __USE_GNU
167	/* Number of each register is the `gregset_t' array.  */
168	enum
169	{
170	  REG_GS = 0,
171	# define REG_GS                REG_GS
172	  REG_FS,
173	# define REG_FS                REG_FS
174	  REG_ES,
175	# define REG_ES                REG_ES
176	  REG_DS,
177	# define REG_DS                REG_DS
178	  REG_EDI,
179	# define REG_EDI        REG_EDI
180	  REG_ESI,
181	# define REG_ESI        REG_ESI
182	  REG_EBP,
183	# define REG_EBP        REG_EBP
184	  REG_ESP,
185	# define REG_ESP        REG_ESP
186	  REG_EBX,
187	# define REG_EBX        REG_EBX
188	  REG_EDX,
189	# define REG_EDX        REG_EDX
190	  REG_ECX,
191	# define REG_ECX        REG_ECX
192	  REG_EAX,
193	# define REG_EAX        REG_EAX
194	  REG_TRAPNO,
195	# define REG_TRAPNO        REG_TRAPNO
196	  REG_ERR,
197	# define REG_ERR        REG_ERR
198	  REG_EIP,
199	# define REG_EIP        REG_EIP
200	  REG_CS,
201	# define REG_CS                REG_CS
202	  REG_EFL,
203	# define REG_EFL        REG_EFL
204	  REG_UESP,
205	# define REG_UESP        REG_UESP
206	  REG_SS
207	# define REG_SS        REG_SS
208	};
209	#endif
210	
211	/* Definitions taken from the kernel headers.  */
212	struct _libc_fpreg
213	{
214	  unsigned short int __ctx(significand)[4];
215	  unsigned short int __ctx(exponent);
216	};
217	
218	struct _libc_fpstate
219	{
220	  unsigned long int __ctx(cw);
221	  unsigned long int __ctx(sw);
222	  unsigned long int __ctx(tag);
223	  unsigned long int __ctx(ipoff);
224	  unsigned long int __ctx(cssel);
225	  unsigned long int __ctx(dataoff);
226	  unsigned long int __ctx(datasel);
227	  struct _libc_fpreg _st[8];
228	  unsigned long int __ctx(status);
229	};
230	
231	/* Structure to describe FPU registers.  */
232	typedef struct _libc_fpstate *fpregset_t;
233	
234	/* Context to describe whole processor state.  */
235	typedef struct
236	  {
237	    gregset_t __ctx(gregs);
238	    /* Due to Linux's history we have to use a pointer here.  The SysV/i386
239	       ABI requires a struct with the values.  */
240	    fpregset_t __ctx(fpregs);
241	    unsigned long int __ctx(oldmask);
242	    unsigned long int __ctx(cr2);
243	  } mcontext_t;
244	
245	/* Userlevel context.  */
246	typedef struct ucontext_t
247	  {
248	    unsigned long int __ctx(uc_flags);
249	    struct ucontext_t *uc_link;
250	    stack_t uc_stack;
251	    mcontext_t uc_mcontext;
252	    sigset_t uc_sigmask;
253	    struct _libc_fpstate __fpregs_mem;
254	  } ucontext_t;
255	
256	#endif /* !__x86_64__ */
257	
258	#undef __ctx
259	
260	#endif /* sys/ucontext.h */
