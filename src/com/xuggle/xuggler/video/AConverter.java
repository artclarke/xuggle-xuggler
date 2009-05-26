/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.video;

import java.awt.image.BufferedImage;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IPixelFormat;

/** An abstract converter class from which specific converters can be
 * derived to do the actual conversions.  This class establishes if
 * the {@link IVideoPicture} needs to be re-sampled, and
 * if so, creates appropriate {@link IVideoResampler} objects to do
 * that.
 */

abstract public class AConverter implements IConverter
{
  /** Re-sampler called when converting image to picture, may be null. */

  protected IVideoResampler mToPictureResampler = null;
  
  /** Re-sampler called when converting picture to image, may be null. */

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
   * Construct an abstract Converter.  This will create a
   * {@link IVideoResampler}
   * to change color-space or resize the picture as needed for the
   * conversions specified.
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
        imageWidth, imageHeight, requiredPictureType,
        mPictureWidth, mPictureHeight, pictureType);
      
      if (mToImageResampler == null)
        throw new RuntimeException(
          "1 Could not create could resampler to translate from " + 
          pictureType + " to " + requiredPictureType + ".");

      mToPictureResampler = IVideoResampler.make(
        mPictureWidth, mPictureHeight, pictureType,
        imageWidth, imageHeight, requiredPictureType);
      
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
   * Return the Type which matches the {@link
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
   * Conditionally re-sample during conversion to picture.
   *
   * @param picture the picture to conditionally re-sample
   *
   * @return if willResample() returns true, the re-sample picture, else
   *         the original picture
   */

  protected IVideoPicture toPictureResample(IVideoPicture picture)
  {
    return willResample() 
      ? resample(picture, mToPictureResampler)
      : picture;
  }

  /**
   * Conditionally re-sample during conversion to image.
   *
   * @param picture the picture to conditionally re-sample
   *
   * @return if willResample() returns true, the re-sample picture, else
   *         the original picture
   */
  
  protected IVideoPicture toImageResample(IVideoPicture picture)
  {
    return willResample()
      ? resample(picture, mToImageResampler)
      : picture;
  }

  /** 
   * Re-sample a picture.
   * 
   * @param picture1 the picture to re-sample
   * @param resampler the picture re-samper to use
   *
   * @throws RuntimeException if could not re-sample picture
   **/

  protected static IVideoPicture resample(IVideoPicture picture1,
    IVideoResampler resampler)
  {
    // create new picture object

    IVideoPicture picture2 = IVideoPicture.make(
      resampler.getOutputPixelFormat(),
      resampler.getOutputWidth(),
      resampler.getOutputHeight());

    // resample

    if (resampler.resample(picture2, picture1) < 0)
      throw new RuntimeException(
        "could not resample from " + resampler.getInputPixelFormat() +
        " to " + resampler.getOutputPixelFormat() + 
        " for picture of type " + picture1.getPixelType());

    // test that it worked

    if (picture2.getPixelType() != resampler.getOutputPixelFormat() 
      || !picture2.isComplete())
    {
      throw new RuntimeException(
        "did not resample from " + resampler.getInputPixelFormat() +
        " to " + resampler.getOutputPixelFormat() +
        " for picture of type " + picture1.getPixelType());
    }

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
