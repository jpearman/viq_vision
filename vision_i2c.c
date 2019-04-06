/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*                        Copyright (c) James Pearman                          */
/*                                   2019                                      */
/*                            All Rights Reserved                              */
/*                                                                             */
/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    Module:     vision_i2c.c                                                 */
/*    Author:     James Pearman                                                */
/*    Created:    6 April 2019                                                 */
/*                                                                             */
/*    Revisions:                                                               */
/*                V1.00     6 April 2019 - Initial release                     */
/*                                                                             */
/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    The author is supplying this software for use with the VEX IQ            */
/*    control system. This file can be freely distributed and teams are        */
/*    authorized to freely use this program , however, it is requested that    */
/*    improvements or additions be shared with the Vex community via the vex   */
/*    forum.  Please acknowledge the work of the authors when appropriate.     */
/*    Thanks.                                                                  */
/*                                                                             */
/*    Licensed under the Apache License, Version 2.0 (the "License");          */
/*    you may not use this file except in compliance with the License.         */
/*    You may obtain a copy of the License at                                  */
/*                                                                             */
/*      http://www.apache.org/licenses/LICENSE-2.0                             */
/*                                                                             */
/*    Unless required by applicable law or agreed to in writing, software      */
/*    distributed under the License is distributed on an "AS IS" BASIS,        */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*    See the License for the specific language governing permissions and      */
/*    limitations under the License.                                           */
/*                                                                             */
/*    The author can be contacted on the vex forums as jpearman                */
/*    or electronic mail using jbpearman_at_mac_dot_com                        */
/*    Mentor for team 8888 RoboLancers, Pasadena CA.                           */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

#ifndef __VISION_I2C__
#define __VISION_I2C__

#define   vexIQ_SensorVision        0x0B

#define   VISION_ID_REG             0x24
#define   VISION_DATA_REG           0x26
#define   VISION_SIGNATURE_REG      0xAF
#define   VISION_MAX_OBJECTS        4
#define   VISION_OBJECTS_DATA_SIZE  6

typedef struct _visionObject {
    short   id;
    short   x;
    short   y;
    short   width;
    short   height;
    short   angle;
    short   total;
} visionObject;

typedef struct _visionSignatue {
    char    id;
    float   range;
    long    uMin;
    long    uMax;
    long    uMean;
    long    vMin;
    long    vMax;
    long    vMean;
    long    mRgb;
    long    mType;
} visionSignature;

/*-----------------------------------------------------------------------------*/
/** @brief  Read objects from the vision sensor                                */
/** @param[in] port the port number on the IQ to use                           */
/** @param[in] id the signature id to request                                  */
/** @param[in] pObject pointer to vision object structure (or array)           */
/** @param[in] len max objects to read - limit 4                               */
/*-----------------------------------------------------------------------------*/
//
// id should be either a signature id in the range 1-7 or a valid color code
// id in octal
//
int
visionObjectGet( portName port, int id, visionObject *pObject, int len ) {
    char    buffer[VISION_MAX_OBJECTS * VISION_OBJECTS_DATA_SIZE];

    if( id <= 0 )
      return(0);

    // limit to VISION_MAX_OBJECTS
    if( len > VISION_MAX_OBJECTS )
      len = VISION_MAX_OBJECTS;

    // did we need to set msb ??
    buffer[0] =  id       & 0xFF;
    buffer[1] = (id >> 8) & 0xFF;

    // ask for object
    genericI2cWrite( port, VISION_ID_REG, buffer, 2 );

    // max data to read
    int  nData = len * VISION_OBJECTS_DATA_SIZE;

    // now read answer
    // try and read 4 objects
    genericI2cRead( port, VISION_DATA_REG, &buffer[0], nData );

    // total objects
    int total = 0;

    // we have to assume call has enough memory allocated
    visionObject *pObj = pObject;

    // loop through all
    for(int i=0;i<len;i++ ) {
      int offset = i*6;

      // object id of 0xFF means no more objects
      if( buffer[offset] == 0xFF )
        break;

      // copy data
      pObj->id     = id;
      pObj->x      = buffer[ offset + 0 ] * 2;
      pObj->y      = buffer[ offset + 1 ];
      pObj->width  = buffer[ offset + 2 ] * 2;
      pObj->height = buffer[ offset + 3 ];
      pObj->angle  = buffer[ offset + 4 ] + (buffer[ offset + 5 ] << 8);

      // nect object
      total++;
      pObj++;
    }

    // return total
    return( total );
}

//
// Helper functions
void
longToBuf( char *buf, long value ) {
    buf[0] =  value        & 0xFF;
    buf[1] = (value >>  8) & 0xFF;
    buf[2] = (value >> 16) & 0xFF;
    buf[3] = (value >> 24) & 0xFF;
}

long
bufToLong( char *buf ) {
    long value = 0;
    value = (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
    return(value);
}

/*-----------------------------------------------------------------------------*/
/** @brief  Write a signature to the vision sensor                             */
/** @param[in] port the port number on the IQ to use                           */
/** @param[in] pSig pointer to vision signature structure                      */
/*-----------------------------------------------------------------------------*/
//
// pSig->id should be valid and in range 1-7
//
int
visionSignatureSet( portName port, visionSignature *pSig ) {
    char  buffer[sizeof(visionSignature)];

    if( pSig->id <= 0 || pSig->id > 7 )
      return(0);

    buffer[0] = pSig->id;

    long *p = (long *)&pSig->range;
    longToBuf( &buffer[ 1], *p  );

    longToBuf( &buffer[ 5], pSig->uMin  );
    longToBuf( &buffer[ 9], pSig->uMax  );
    longToBuf( &buffer[13], pSig->uMean );
    longToBuf( &buffer[17], pSig->vMin  );
    longToBuf( &buffer[21], pSig->vMax  );
    longToBuf( &buffer[25], pSig->vMean );
    longToBuf( &buffer[29], pSig->mRgb );
    longToBuf( &buffer[33], pSig->mType );

    genericI2cWrite( port, VISION_SIGNATURE_REG, buffer, sizeof(visionSignature) );

    return(1);
}

/*-----------------------------------------------------------------------------*/
/** @brief  Read signature back from the vision sensor                         */
/** @param[in] port the port number on the IQ to use                           */
/** @param[in] pSig pointer to vision signature structure to save result in    */
/*-----------------------------------------------------------------------------*/
//
// pSig->id should be valid and in range 1-7
//
int
visionSignatureGet( portName port, visionSignature *pSig ) {
    char  buffer[sizeof(visionSignature)];

    if( pSig->id <= 0 || pSig->id > 7 )
      return(0);

    buffer[0] = pSig->id;
    // first byte is signature to read
    genericI2cWrite( port, VISION_SIGNATURE_REG, buffer, 1 );
    // read back all data, 36 bytes
    genericI2cRead( port, VISION_SIGNATURE_REG+1, buffer, sizeof(visionSignature)-1 );

    long *p = (long *)&pSig->range;
    *p = bufToLong( &buffer[0] );

    pSig->uMin  = bufToLong( &buffer[ 4] );
    pSig->uMax  = bufToLong( &buffer[ 8] );
    pSig->uMean = bufToLong( &buffer[12] );
    pSig->vMin  = bufToLong( &buffer[16] );
    pSig->vMax  = bufToLong( &buffer[20] );
    pSig->vMean = bufToLong( &buffer[24] );
    pSig->mRgb  = bufToLong( &buffer[28] );
    pSig->mType = bufToLong( &buffer[32] );

    return(sizeof(visionSignature));
}

/*-----------------------------------------------------------------------------*/
/** @brief  Find the first vision sensor installed                             */
/** @returns the port number if found else (-1)                                */
/*-----------------------------------------------------------------------------*/
/**
 * @details
 *  scan all ports looking for an installed vision sensor
 */

portName
visionI2cFindFirst()
{
    TVexIQDeviceTypes   type;
    TDeviceStatus       status;
    short               ver;
    portName            index;

    // Get all device info
    for(index=PORT1;index<=PORT12;index++)
      {
      getVexIqDeviceInfo( index, type, status, ver );
      if( (char)type == vexIQ_SensorVision ) {
        writeDebugStreamLine("found vision sensor on port %d", index);
        return((portName)index);
        }
      }
    return((portName)-1);
}

#endif // __VISION_I2C__
