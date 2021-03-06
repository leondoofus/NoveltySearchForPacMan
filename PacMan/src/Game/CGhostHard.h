#ifndef CGHOSTHARD_H_DEFINED
#define CGHOSTHARD_H_DEFINED

#include <CPlayer.h>
#include <CGhost.h>
#include <CBoard.h>
#include <CWindow.h>

/// Smartest type of ghost, on every crossroad checks where 
/// ghost is and goes that way.
class CGhostHard : public CGhost
	{
	public:
		CGhostHard ( int x, int y );
		virtual ~CGhostHard ();
		
		virtual void draw ( CWindow * window );
		virtual void update ( CBoard * board, CPlayer * player );
	};

	
#endif //CGHOSTHARD_H_DEFINED
