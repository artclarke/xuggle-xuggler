/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package com.xuggle.xuggler.video;

import java.awt.image.BufferedImage;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IPixelFormat;

/** An abstract converter class from which specific converters can be
 * derrived to do the actual conversions.  This class establishes if
 * {@link IVideoPicture} resamplers are needed and if so, creates them.
 */

abstract public class AConverter implements IConverter
{
  /** Resampler called when converting image to picture, may be null. */

  protected IVideoResampler mToPictureResampler = null;
  
  /** Resampler called when converting picture to image, may be null. */

  protected IVideoResampler mToImageResampler = null;

  /** The width of the pictures. */
 
  protected int mPictureWidth;

  /** The height of the pictures. */

  protected int mPictureHeight;

  /** The width of the images. */

  protected int mImageWidth;

  /** The height of the images. */

  protected int mImageHeight;

  // the recognized BufferedImage type

  final private IPixelFormat.Type mPictureType;

  // the recognized BufferedImage type

  final private IPixelFormat.Type mRequiredPictureType;

  // the recognized BufferedImage type

  final private int mImageType;

  // the description of this convert

  final private String mDescription;

  /** 
   * Construct an abstract Converter.  This will create a resampler to
   * change colorspace or resize the picture as needed for the
   * conversions specifed.
   *
   * @param pictureType the recognized {@link IVideoPicture} type
   * @param requiredPictureType the picture type requred to translate to
   *        and from the BufferedImage
   * @param imageType the recognized {@link BufferedImage} type
   * @param pictureWidth the width of picture
   * @param pictureHeight the height of picture
   * @param imageWidth the width of image
   * @param imageHeight the height of image
   */

  public AConverter(
    IPixelFormat.Type pictureType, 
    IPixelFormat.Type requiredPictureType, 
    int imageType,
    int pictureWidth, 
    int pictureHeight,
    int imageWidth, 
    int imageHeight)
  {
    // by default there is no resample description

    String resampleDescription = "";

    // record the image and picture parameters
    
    mPictureType = pictureType;
    mRequiredPictureType = requiredPictureType;
    mImageType = imageType;
    mPictureWidth = pictureWidth;
    mPictureHeight = pictureHeight;
    mImageWidth = imageWidth;
    mImageHeight = imageHeight;

    // if the picture type is not the type or size required, create the
    // resamplers to fix that

    if (!pictureType.equals(requiredPictureType) 
      || (mPictureWidth != mImageWidth)
      || (mPictureHeight != mImageHeight))
    {
      if (!IVideoResampler.isSupported(
          IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
        throw new RuntimeException(
          "0 Could not create could resampler to translate from " + 
          pictureType + " to " + requiredPictureType + ".  " + 
          "Color space conversion is not supported by this version of" +
          "Xuggler.  Recompile Xuggler with the GPL option enabled.");

      mToImageResampler = IVideoResampler.make(
        mPictureWidth, mPictureHeight, requiredPictureType,
        imageWidth, imageHeight, pictureType);

      if (mToImageResampler == null)
        throw new RuntimeException(
          "1 Could not create could resampler to translate from " + 
          pictureType + " to " + requiredPictureType + ".");

      mToPictureResampler = IVideoResampler.make(
        imageWidth, imageHeight, pictureType,
        mPictureWidth, mPictureHeight, requiredPictureType);

      if (mToPictureResampler == null)
        throw new RuntimeException(
          "2 Could not create could resampler to translate from " + 
          requiredPictureType + " to " + pictureType);

      resampleDescription = "Pictures will be resampled to and from " + 
        requiredPictureType + " during translation.";
    }

    // construct the description of this converter

    mDescription = "A converter which translates [" +
      pictureWidth + "x" + pictureHeight + "] IVideoPicture type " + 
      pictureType + " to and from [" + imageWidth + "x" + imageHeight +
      "] BufferedImage type " + imageType + ".  " + resampleDescription;
  }

  /** {@inheritDoc} */

  public IPixelFormat.Type getPictureType()
  {
    return mPictureType;
  }

  /**
   * Return the {@link IPixelFormat.Type} which matches the {@link
   * BufferedImage} type.
   * 
   * @return the picture type which allows for image translation.
   */

  protected IPixelFormat.Type getRequiredPictureType()
  {
    return mRequiredPictureType;
  }

  /** {@inheritDoc} */

  public int getImageType()
  {
    return mImageType;
  }

  /** {@inheritDoc} */

  public boolean willResample()
  {
    return null != mToPictureResampler && null != mToImageResampler;
  }

  /**
   * Conditionally resample during convertion to picture.
   *
   * @param picture the picture to conditionally resample
   *
   * @return if willResample() returns true, the resample picture, else
   *         the origonal picture
   */

  protected IVideoPicture toPictureResample(IVideoPicture picture)
  {
    return willResample() 
      ? resample(picture, mToPictureResampler)
      : picture;
  }

  /**
   * Conditionally resample during convertion to image.
   *
   * @param picture the picture to conditionally resample
   *
   * @return if willResample() returns true, the resample picture, else
   *         the origonal picture
   */
  
  protected IVideoPicture toImageResample(IVideoPicture picture)
  {
    return willResample()
      ? resample(picture, mToImageResampler)
      : picture;
  }

  /** 
   * Resample a picture.
   * 
   * @param picture1 the picture to resample
   * @param resampler the picture resamper to use
   *
   * @throws RuntimeException if could not reample picture
   **/

  protected static IVideoPicture resample(IVideoPicture picture1,
    IVideoResampler resampler)
  {
    // create new picture object

    IVideoPicture picture2 = IVideoPicture.make(
      resampler.getOutputPixelFormat(),
      picture1.getWidth(), picture1.getHeight());

    // resample

    if (resampler.resample(picture2, picture1) < 0)
      throw new RuntimeException(
        "could not resample from " + resampler.getInputPixelFormat() +
        " to " + resampler.getOutputPixelFormat() + 
        " for picture of type " + picture1.getPixelType());

    // test that it worked

    if (picture2.getPixelType() != resampler.getOutputPixelFormat())
      throw new RuntimeException(
        "did not resample from " + resampler.getInputPixelFormat() +
        " to " + resampler.getOutputPixelFormat() +
        " for picture of type " + picture1.getPixelType());

    // return the resample picture

    return picture2;
  }

  /** 
   * Test that the passed image is valid and conforms to the
   * converters specifications.
   *
   * @param image the image to test
   *
   * @throws IllegalArgumentException if the passed {@link
   *         BufferedImage} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   *         BufferedImage} is not the correct type. See {@link
   *         #getImageType}.
   */

  protected void validateImage(BufferedImage image)
  {
    // if the image is NULL, throw up

    if (image == null)
      throw new IllegalArgumentException("The passed image is NULL.");

    // if image is not the correct type, throw up

    if (image.getType() != getImageType())
      throw new IllegalArgumentException(
        "The passed image is of type #" + image.getType() +
        " but is required to be of BufferedImage type #" +
        getImageType() + ".");
  }

  /** 
   * Test that the passed picture is valid and conforms to the
   * converters specifications.
   *
   * @param picture the picture to test
   *
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is not complete.
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is not the correct type.
   */

  protected void validatePicture(IVideoPicture picture)
  {
    // if the pictre is NULL, throw up
    
    if (picture == null)
      throw new IllegalArgumentException("The picture is NULL.");

    // if the picture is not complete, throw up
    
    if (!picture.isComplete())
      throw new IllegalArgumentException("The picture is not complete.");

    // if the picture is an invalid type throw up

    IPixelFormat.Type type = picture.getPixelType();
    if ((type != getPictureType()) && (willResample() && 
        type != mToImageResampler.getOutputPixelFormat()))
      throw new IllegalArgumentException(
        "Picture is of type: " + type + ", but must be " + 
        getPictureType() + (willResample() 
          ? " or " + mToImageResampler.getOutputPixelFormat()
          : "") +
        ".");
  }

  /** {@inheritDoc} */

  public String getDescription()
  {
    return mDescription;
  }

  /** Get a string representation of this converter. */

  public String toString()
  {
    return getDescription();
  }

}
