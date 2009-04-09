/*
Java wrapper for libdmtx

Copyright (C) 2009 Pete Calvert
Copyright (C) 2009 Dikran Seropian
Copyright (C) 2009 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

/* $Id: org_libdmtx_DMTXImage.c 726 2009-02-19 22:17:48Z mblaughton $ */

#include "org_libdmtx_DMTXImage.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <dmtx.h>

/**
 * Construct from ID (static factory method since JNI doesn't allow native
 * constructors).
 */
JNIEXPORT jobject JNICALL
Java_org_libdmtx_DMTXImage_createTag(JNIEnv *aEnv, jclass aClass, jstring aID)
{
   DmtxEncode *lEncoded;
   jclass      lImageClass;
   jmethodID   lConstructor;
   jobject     lResult;
   jintArray   lJavaData;
   int         lW, lH, lBPP, lI, lJ;
   jint       *lPixels;

   /* Convert ID into string */
   const char *sStrID = (*aEnv)->GetStringUTFChars(aEnv, aID, NULL);

   /* Create Data Matrix */
   lEncoded = dmtxEncodeCreate();
   dmtxEncodeDataMatrix(lEncoded, strlen(sStrID), (unsigned char *)sStrID);

   /* Finished with ID, so release it */
   (*aEnv)->ReleaseStringUTFChars(aEnv, aID, sStrID);

   dmtxEncodeSetProp(lEncoded, DmtxPropPixelPacking, DmtxPack32bppRGBX);
   dmtxEncodeSetProp(lEncoded, DmtxPropImageFlip, DmtxFlipNone);

   /* Find DMTXImage class */
   lImageClass = (*aEnv)->FindClass(aEnv, "org/libdmtx/DMTXImage");
   if(lImageClass == NULL)
      return NULL;

   /* Find constructor */
   lConstructor = (*aEnv)->GetMethodID(aEnv, lImageClass, "<init>", "(II[I)V");
   if(lConstructor == NULL)
      return NULL;

   /* Get properties from image */
   lW = dmtxImageGetProp(lEncoded->image, DmtxPropWidth);
   lH = dmtxImageGetProp(lEncoded->image, DmtxPropHeight);
   lBPP = dmtxImageGetProp(lEncoded->image, DmtxPropBytesPerPixel);

   if(lBPP != 3)
      return NULL;

   /* Copy Pixel Data (converting to RGB as we go and also doing Y flip) */
   lJavaData = (*aEnv)->NewIntArray(aEnv, lW * lH);
   lPixels = (*aEnv)->GetIntArrayElements(aEnv, lJavaData, NULL);

   for(lI = 0; lI < lH; lI++) {
      for(lJ = 0; lJ < lW; lJ++) {
         int lOldIndex = 3 * ((lI * lW) + lJ);
         int lNewIndex = ((lH - lI - 1) * lW) + lJ;
         lPixels[lNewIndex] = (lEncoded->image->pxl[lOldIndex + 2] << 16)
                                     | (lEncoded->image->pxl[lOldIndex + 1] <<   8)
                                     | (lEncoded->image->pxl[lOldIndex + 0]         );
      }
   }

/* new version that I haven't actually tried yet:
   memcpy(lPixels, lEncoded->image->pxl, lW * lH * 4);
*/
   (*aEnv)->ReleaseIntArrayElements(aEnv, lJavaData, lPixels, 0);

   /* Create Image instance */
   lResult = (*aEnv)->NewObject(aEnv, lImageClass, lConstructor, lW, lH, lJavaData);
   if(lResult == NULL)
      return NULL;

   /* Destroy original image */
   dmtxEncodeDestroy(&lEncoded);

   /* Free local references */
   (*aEnv)->DeleteLocalRef(aEnv, lJavaData);
   (*aEnv)->DeleteLocalRef(aEnv, lImageClass);

   return lResult;
}

/**
 * Decode the image, returning tags found (as DMTXTag objects)
 */
JNIEXPORT jobjectArray JNICALL
Java_org_libdmtx_DMTXImage_getTags(JNIEnv *aEnv, jobject aImage,
      jint aTagCount, jint lSearchTimeout)
{
   jclass        lImageClass, lTagClass, lPointClass;
   jmethodID     lTagConstructor, lPointConstructor;
   jfieldID      lWidth, lHeight, lData;
   DmtxImage    *lImage;
   DmtxDecode   *lDecode;
   DmtxRegion   *lRegion;
   DmtxTime      lTimeout;
   int           lW, lH, lI;
   jintArray     lJavaData;
   jint         *lPixels;
   jobject      *lTags;
   jobjectArray  lResult;
   int           lTagCount = 0;

   /* Find DMTXImage class */
   lImageClass = (*aEnv)->FindClass(aEnv, "org/libdmtx/DMTXImage");
   if(lImageClass == NULL)
      return NULL;

   /* Find Tag class */
   lTagClass = (*aEnv)->FindClass(aEnv, "org/libdmtx/DMTXTag");
   if(lTagClass == NULL)
      return NULL;

   /* Find Point class */
   lPointClass = (*aEnv)->FindClass(aEnv, "java/awt/Point");
   if(lPointClass == NULL)
      return NULL;

   /* Find constructors */
   lTagConstructor = (*aEnv)->GetMethodID(
      aEnv, lTagClass, "<init>",
      "(Ljava/lang/String;Ljava/awt/Point;Ljava/awt/Point;Ljava/awt/Point;Ljava/awt/Point;)V"
   );

   lPointConstructor = (*aEnv)->GetMethodID(aEnv, lPointClass, "<init>", "(II)V");
   if(lTagConstructor == NULL || lPointConstructor == NULL)
      return NULL;

   /* Find fields */
   lWidth = (*aEnv)->GetFieldID(aEnv, lImageClass, "width", "I");
   lHeight = (*aEnv)->GetFieldID(aEnv, lImageClass, "height", "I");
   lData = (*aEnv)->GetFieldID(aEnv, lImageClass, "data", "[I");

   if(lWidth == NULL || lHeight == NULL || lData == NULL)
      return NULL;

   /* Get fields */
   lW = (*aEnv)->GetIntField(aEnv, aImage, lWidth);
   lH = (*aEnv)->GetIntField(aEnv, aImage, lHeight);

   lJavaData = (*aEnv)->GetObjectField(aEnv, aImage, lData);
   lPixels = (*aEnv)->GetIntArrayElements(aEnv, lJavaData, NULL);

   /* Create DmtxImage object */
   lImage = dmtxImageCreate((unsigned char *)lPixels, lW, lH, DmtxPack32bppRGBX);
   if(lImage == NULL)
      return NULL;

   /* Create DmtxDecode object */
   lDecode = dmtxDecodeCreate(lImage, 1);
   if(lDecode == NULL)
      return NULL;

   /* Allocate temporary Tag array */
   lTags = (jobject *)malloc(aTagCount * sizeof(jobject));
   if(lTags == NULL)
      return NULL;

   /* Find all tags that we can inside timeout */
   lTimeout = dmtxTimeAdd(dmtxTimeNow(), lSearchTimeout);

   while(lTagCount < aTagCount && (lRegion = dmtxRegionFindNext(lDecode, &lTimeout))) {
      jstring sStringID;
      DmtxMessage *lMessage = dmtxDecodeMatrixRegion(lDecode, lRegion, DmtxUndefined);

      if(lMessage != NULL) {
         DmtxVector2 lCorner1, lCorner2, lCorner3, lCorner4;
         jobject lJCorner1, lJCorner2, lJCorner3, lJCorner4;

         /* Calculate position of Tag */
         lCorner1.X = lCorner1.Y = lCorner2.Y = lCorner4.X = 0.0;
         lCorner2.X = lCorner4.Y = lCorner3.X = lCorner3.Y = 1.0;

         dmtxMatrix3VMultiplyBy(&lCorner1, lRegion->fit2raw);
         dmtxMatrix3VMultiplyBy(&lCorner2, lRegion->fit2raw);
         dmtxMatrix3VMultiplyBy(&lCorner3, lRegion->fit2raw);
         dmtxMatrix3VMultiplyBy(&lCorner4, lRegion->fit2raw);

         /* Create Location instances for corners */
         lJCorner1 = (*aEnv)->NewObject(aEnv, lPointClass, lPointConstructor,
               (int) lCorner1.X, (int) (lH - lCorner1.Y - 1));

         lJCorner2 = (*aEnv)->NewObject(aEnv, lPointClass, lPointConstructor,
               (int) lCorner2.X, (int) (lH - lCorner2.Y - 1));

         lJCorner3 = (*aEnv)->NewObject(aEnv, lPointClass, lPointConstructor,
               (int) lCorner3.X, (int) (lH - lCorner3.Y - 1));

         lJCorner4 = (*aEnv)->NewObject(aEnv, lPointClass, lPointConstructor,
               (int) lCorner4.X, (int) (lH - lCorner4.Y - 1));

         /* Decode Message */
         sStringID = (*aEnv)->NewStringUTF(aEnv, lMessage->output);

         /* Create Tag instance */
         lTags[lTagCount] = (*aEnv)->NewObject(aEnv, lTagClass, lTagConstructor,
               sStringID, lJCorner1, lJCorner2, lJCorner3, lJCorner4);
         if(lTags[lTagCount] == NULL)
            return NULL;

         /* Increment Count */
         lTagCount++;

         /* Free Message */
         dmtxMessageDestroy(&lMessage);
      }

      /* Free Region */
      dmtxRegionDestroy(&lRegion);
   }

   /* Free DMTX Structures */
   dmtxDecodeDestroy(&lDecode);
   dmtxImageDestroy(&lImage);

   /* Release Image Data */
   (*aEnv)->ReleaseIntArrayElements(aEnv, lJavaData, lPixels, 0);

   /* Create result array */
   lResult = (*aEnv)->NewObjectArray(aEnv, lTagCount, lTagClass, NULL);

   for(lI = 0; lI < lTagCount; lI++) {
      (*aEnv)->SetObjectArrayElement(aEnv, lResult, lI, lTags[lI]);
   }

   /* Free local references */
   (*aEnv)->DeleteLocalRef(aEnv, lJavaData);
   (*aEnv)->DeleteLocalRef(aEnv, lImageClass);
   (*aEnv)->DeleteLocalRef(aEnv, lTagClass);
   (*aEnv)->DeleteLocalRef(aEnv, lPointClass);

   return lResult;
}
