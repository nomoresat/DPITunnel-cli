/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_EMATCH_ROUTE_CLS_EMATCH_SYNTAX_H_INCLUDED
# define YY_EMATCH_ROUTE_CLS_EMATCH_SYNTAX_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ematch_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ERROR = 258,
    LOGIC = 259,
    NOT = 260,
    OPERAND = 261,
    NUMBER = 262,
    ALIGN = 263,
    LAYER = 264,
    KW_OPEN = 265,
    KW_CLOSE = 266,
    KW_PLUS = 267,
    KW_MASK = 268,
    KW_SHIFT = 269,
    KW_AT = 270,
    EMATCH_CMP = 271,
    EMATCH_NBYTE = 272,
    EMATCH_TEXT = 273,
    EMATCH_META = 274,
    KW_EQ = 275,
    KW_GT = 276,
    KW_LT = 277,
    KW_FROM = 278,
    KW_TO = 279,
    META_RANDOM = 280,
    META_LOADAVG_0 = 281,
    META_LOADAVG_1 = 282,
    META_LOADAVG_2 = 283,
    META_DEV = 284,
    META_PRIO = 285,
    META_PROTO = 286,
    META_PKTTYPE = 287,
    META_PKTLEN = 288,
    META_DATALEN = 289,
    META_MACLEN = 290,
    META_MARK = 291,
    META_TCINDEX = 292,
    META_RTCLASSID = 293,
    META_RTIIF = 294,
    META_SK_FAMILY = 295,
    META_SK_STATE = 296,
    META_SK_REUSE = 297,
    META_SK_REFCNT = 298,
    META_SK_RCVBUF = 299,
    META_SK_SNDBUF = 300,
    META_SK_SHUTDOWN = 301,
    META_SK_PROTO = 302,
    META_SK_TYPE = 303,
    META_SK_RMEM_ALLOC = 304,
    META_SK_WMEM_ALLOC = 305,
    META_SK_WMEM_QUEUED = 306,
    META_SK_RCV_QLEN = 307,
    META_SK_SND_QLEN = 308,
    META_SK_ERR_QLEN = 309,
    META_SK_FORWARD_ALLOCS = 310,
    META_SK_ALLOCS = 311,
    META_SK_ROUTE_CAPS = 312,
    META_SK_HASH = 313,
    META_SK_LINGERTIME = 314,
    META_SK_ACK_BACKLOG = 315,
    META_SK_MAX_ACK_BACKLOG = 316,
    META_SK_PRIO = 317,
    META_SK_RCVLOWAT = 318,
    META_SK_RCVTIMEO = 319,
    META_SK_SNDTIMEO = 320,
    META_SK_SENDMSG_OFF = 321,
    META_SK_WRITE_PENDING = 322,
    META_VLAN = 323,
    META_RXHASH = 324,
    META_DEVNAME = 325,
    META_SK_BOUND_IF = 326,
    STR = 327,
    QUOTED = 328
  };
#endif
/* Tokens.  */
#define ERROR 258
#define LOGIC 259
#define NOT 260
#define OPERAND 261
#define NUMBER 262
#define ALIGN 263
#define LAYER 264
#define KW_OPEN 265
#define KW_CLOSE 266
#define KW_PLUS 267
#define KW_MASK 268
#define KW_SHIFT 269
#define KW_AT 270
#define EMATCH_CMP 271
#define EMATCH_NBYTE 272
#define EMATCH_TEXT 273
#define EMATCH_META 274
#define KW_EQ 275
#define KW_GT 276
#define KW_LT 277
#define KW_FROM 278
#define KW_TO 279
#define META_RANDOM 280
#define META_LOADAVG_0 281
#define META_LOADAVG_1 282
#define META_LOADAVG_2 283
#define META_DEV 284
#define META_PRIO 285
#define META_PROTO 286
#define META_PKTTYPE 287
#define META_PKTLEN 288
#define META_DATALEN 289
#define META_MACLEN 290
#define META_MARK 291
#define META_TCINDEX 292
#define META_RTCLASSID 293
#define META_RTIIF 294
#define META_SK_FAMILY 295
#define META_SK_STATE 296
#define META_SK_REUSE 297
#define META_SK_REFCNT 298
#define META_SK_RCVBUF 299
#define META_SK_SNDBUF 300
#define META_SK_SHUTDOWN 301
#define META_SK_PROTO 302
#define META_SK_TYPE 303
#define META_SK_RMEM_ALLOC 304
#define META_SK_WMEM_ALLOC 305
#define META_SK_WMEM_QUEUED 306
#define META_SK_RCV_QLEN 307
#define META_SK_SND_QLEN 308
#define META_SK_ERR_QLEN 309
#define META_SK_FORWARD_ALLOCS 310
#define META_SK_ALLOCS 311
#define META_SK_ROUTE_CAPS 312
#define META_SK_HASH 313
#define META_SK_LINGERTIME 314
#define META_SK_ACK_BACKLOG 315
#define META_SK_MAX_ACK_BACKLOG 316
#define META_SK_PRIO 317
#define META_SK_RCVLOWAT 318
#define META_SK_RCVTIMEO 319
#define META_SK_SNDTIMEO 320
#define META_SK_SENDMSG_OFF 321
#define META_SK_WRITE_PENDING 322
#define META_VLAN 323
#define META_RXHASH 324
#define META_DEVNAME 325
#define META_SK_BOUND_IF 326
#define STR 327
#define QUOTED 328

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 35 "route/cls/ematch_syntax.y"

	struct tcf_em_cmp	cmp;
	struct ematch_quoted	q;
	struct rtnl_ematch *	e;
	struct rtnl_pktloc *	loc;
	struct rtnl_meta_value *mv;
	uint32_t		i;
	uint64_t		i64;
	char *			s;

#line 214 "route/cls/ematch_syntax.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int ematch_parse (void *scanner, char **errp, struct nl_list_head *root);

#endif /* !YY_EMATCH_ROUTE_CLS_EMATCH_SYNTAX_H_INCLUDED  */
