/*
	$Id: keycodes.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef KEYCODES_H
#define KEYCODES_H

#define cntrl(c) ((c) & 037)
#define meta(c) ((c) | 0400)

#ifndef DJGPP

#define kbUp     meta( 'u' )
#define kbDown   meta( 'd' )
#define kbLeft   meta( 'l' )
#define kbRight  meta( 'r' )
#define kbPgUp   meta( 'p' )
#define kbPgDn   meta( 'n' )
#define kbHome   meta( 'h' )
#define kbEnd    meta( 'e' )
#define kbDel    meta( 'x' )
#define kbIns    meta( 'i' )

#define kbF1	meta( 'A' )
#define kbF2	meta( 'B' )
#define kbF3	meta( 'C' )
#define kbF4	meta( 'D' )
#define kbF5	meta( 'E' )
#define kbF6	meta( 'F' )
#define kbF7	meta( 'G' )
#define kbF8	meta( 'H' )
#define kbF9	meta( 'I' )
#define kbF10	meta( 'J' )
#define kbF11	meta( 'K' )
#define kbF12	meta( 'L' )

#else

#define kbUp     meta( 'H' )
#define kbDown   meta( 'P' )
#define kbLeft   meta( 'K' )
#define kbRight  meta( 'M' )
#define kbPgUp   meta( 'I' )
#define kbPgDn   meta( 'Q' )
#define kbHome   meta( 'G' )
#define kbEnd    meta( 'O' )
#define kbDel    meta( 'S' )
#define kbIns    meta( 'R' )

#define kbF1	meta( ';' )
#define kbF2	meta( '<' )
#define kbF3	meta( '=' )
#define kbF4	meta( '>' )
#define kbF5	meta( '?' )
#define kbF6	meta( '@' )
#define kbF7	meta( 'A' )
#define kbF8	meta( 'B' )
#define kbF9	meta( 'C' )
#define kbF10	meta( 'D' )
#define kbF11	meta( 'å' )
#define kbF12	meta( 'ö' )

#endif

#define kbEnter   0x0d
#define kbCtrlEnter   0x0a
#define kbTab     0x09
#define kbBS      0x08
#define BELL      0x07
#define kbEsc     0x1b

#define    kbCtrlA    0x0001
#define    kbCtrlB    0x0002
#define    kbCtrlC    0x0003
#define    kbCtrlD    0x0004
#define    kbCtrlE    0x0005
#define    kbCtrlF    0x0006
#define    kbCtrlG    0x0007
#define    kbCtrlH    0x0008
#define    kbCtrlI    0x0009
#define    kbCtrlJ    0x000a
#define    kbCtrlK    0x000b
#define    kbCtrlL    0x000c
#define    kbCtrlM    0x000d
#define    kbCtrlN    0x000e
#define    kbCtrlO    0x000f
#define    kbCtrlP    0x0010
#define    kbCtrlQ    0x0011
#define    kbCtrlR    0x0012
#define    kbCtrlS    0x0013
#define    kbCtrlT    0x0014
#define    kbCtrlU    0x0015
#define    kbCtrlV    0x0016
#define    kbCtrlW    0x0017
#define    kbCtrlX    0x0018
#define    kbCtrlY    0x0019
#define    kbCtrlZ    0x001a
#define    kbCtrlSlash 0x001c
#define    kbCtrlBr    0x001d
#define    kbCtrlKr    0x001e
#define    kbCtrl_    0x001f

#define FUNC(key)	((key)|0x200)
#define FUNC1(key)	((key)|0x200)
#define FUNC2(key)	((key)|0x400)
#define FUNC12(key)	((key)|0x600)
#define KEYMASK(key)    ((key)|0x7ff)
#define ISKEY(key)	(!((key)&~0x7ff))
#define ONEKEY(key)	((key)&0x1ff)
#define ISFUNC1(key)	(((key)&0x600)==0x200)
#define ISFUNC2(key)	(((key)&0x600)==0x400)
#define ISFUNC12(key)	(((key)&0x600)==0x600)

const char *_descriptKey( int key );

#endif
