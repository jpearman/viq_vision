//
// Demo code for ROBOTC user space vision sensor access
//
// James Pearman, 6 April 2019
//

#include "generic_i2c.c"
#include "vision_i2c.c"

visionObject     obj[VISION_MAX_OBJECTS];

task main()
{
    portName    port;

    eraseDisplay();

    port = visionI2cFindFirst();

    if( port >= 0 ) {
      writeDebugStreamLine("Found vision sensor on port %d", port );
      displayString( 0, "Using Port %d", port+1 );

      while(1) {
        int nObjects = visionObjectGet( port, 1, obj, 4 );

        if( nObjects > 0 ) {
          writeDebugStreamLine("found %d", nObjects );
          displayString( 1, "Objects %d       ", nObjects );
          displayString( 2, "Object0 X: %3d Y: %3d", obj[0].x, obj[0].y );
          displayString( 3, "Object0 W: %3d H: %3d", obj[0].width, obj[0].height );
          for(int i=0;i<nObjects;i++) {
            writeDebugStreamLine("%d: %3d %3d %3d %3d %3d",i, obj[i].id, obj[i].x, obj[i].y, obj[i].width, obj[i].height );
          }
        }
        else {
          displayTextLine( 1, "No objects found" );
          displayTextLine( 2, "" );
          displayTextLine( 3, "" );
        }

        wait1Msec(200);
      }
    }
    else {
      displayString( 0, "No vision sensor" );
    }
}
