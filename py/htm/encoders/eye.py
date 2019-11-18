# Written by David McDougall, 2018

"""
From Wikipedia "Retina": Although there are more than 130 million
retinal receptors, there are only approximately 1.2 million fibres
(axons) in the optic nerve; a large amount of pre-processing is
performed within the retina. The fovea produces the most accurate
information. Despite occupying about 0.01% of the visual field (less
than 2 of visual angle), about 10% of axons in the optic nerve are
devoted to the fovea.

For more information on visual processing on retina, see 
https://foundationsofvision.stanford.edu/chapter-5-the-retinal-representation/#visualinformation


Fun Fact 1: The human optic nerve has 800,000 ~ 1,700,000 nerve fibers.
Fun Fact 2: The human eye can distiguish between 10 million different colors.
~Sources: Wikipedia. 
"""

import math
import random

import numpy as np
import cv2 # pip install opencv-contrib-python

from htm.bindings.sdr import SDR


class ChannelEncoder:
    """
    This assigns a random range to each bit of the output SDR.  Each bit becomes
    active if its corresponding input falls in its range.  By using random
    ranges, each bit represents a different thing even if it mostly overlaps
    with other comparable bits.  This way redundant bits add meaning.

    Further explanation for this encoder (which is currently only used by 
    Retina): 

    Two requirements for encoding SDRs are that every bit of information represents
    a range of possible values, and that for every input multiple bits activate in the SDR. 
    The effects of these requirements are that each output bit is inaccurate and redundant. 
    This encoder makes every output bit receptive to a unique range of inputs, 
    which are uniformly distributed throughout the input space. This meets the requirements 
    for being an SDR and has more representational power than if many of the bits 
    represented the identical input ranges. 
    This design makes all of those redundancies add useful information.

    1. Semantic similarity happens when two inputs which are similar have similar SDR representations. 
    This encoder design does two things to cause semantic similarity: 
    (1) SDR bits are responsive to a range of input values, 
    (2) topology allows near by bits to represent similar things.

    Many encoders apply thresholds to real valued input data to convert the input 
    into Boolean outputs. In this encoder uses two thresholds to form ranges which 
    are referred to as ‘bins’. A small change in the input value might cause some 
    of the output bits to change and a large change in input value will cause all 
    of the output bits to change. How sensitive the output bits are to changes 
    in the input value -the semantic similarity- is determined by the sizes of the bins. 
    The sizes of the bins are in turn determined by the sparsity, as follows:
    - Assume that the inputs are distributed in a uniform random way throughout the input range. 
    - The size of the bins then determines the probability that an input value will fall inside of a bin. 
    This means that the sparsity is related to the size of the bins, 
    which in turn means that the sparsity is related to the amount of semantic similarity. 
    This may seem counter-intuitive but this same property holds true for all 
    encoders which use bins to convert real numbers into discrete bits.

    2. This encoder relies on topology in the input image, 
    the idea that adjacent pixels in the input image are likely to show the same thing. 
    If an area of the output SDR can not represent a color because it did not generate
    the bins needed to, then it may still be near to a pixel which does represent the color. 
    In the case where there are less than one active output bits per input pixel, 
    multiple close together outputs can work together to represent the input. 
    Also, if an output bit changes in response to a small change in input, 
    then some semantic similarity is lost. 
    Topology allows nearby outputs to represent the same thing, 
    and since each output uses random bins they will not all change when 
    the input reaches a single threshold.

    Note about sparsity and color channels:
      To encode color images create separate encoders for each color channel. 
      Then recombine the output SDRs into a single monolithic SDR by multiplying them together. 
      Multiplication is equivalent to logical “and” in this situation. 
      Notice that the combined SDR’s sparsity is the different; the fraction of bits which 
      are active in the combined SDR is the product of the fraction of the bits 
      which are active in all input SDRs. 
      For example, to make an encoder with 8 bits per pixel and a sparsity of 1/8: 
      create three encoders with 8 bits per pixel and a sparsity of 1/2.
 
    For more on encoding see: 
    Encoding Data for HTM Systems, Scott Purdy 2016, 
    https://arxiv.org/abs/1602.05925
    """

    def __init__(self, input_shape, num_samples, sparsity,
        dtype       = np.float64,
        drange      = range(0,1),
        wrap        = False):
        """
        Argument input_shape is tuple of dimensions for each input frame.

        Argument num_samples is number of bits in the output SDR which will
                 represent each input number, this is the added data depth.

        Argument sparsity is fraction of output which on average will be active.
                 This is also the fraction of the input spaces which (on 
                 average) each bin covers.

        Argument dtype is numpy data type of channel.

        Argument drange is a range object or a pair of values representing the 
                 range of possible channel values.

        Argument wrap ... default is False.
                 This supports modular input spaces and ranges which wrap
                 around. It does this by rotating the inputs by a constant
                 random amount which hides where the discontinuity in ranges is.
                 No ranges actually wrap around the input space.
        """
        self.input_shape  = tuple(input_shape)
        self.num_samples  = int(round(num_samples))
        self.sparsity     = sparsity
        self.dimensions   = self.input_shape + (self.num_samples,) # output shape
        self.size         = np.prod(self.dimensions)
        self.dtype        = dtype
        self.drange       = drange
        self.len_drange   = max(drange) - min(drange)
        self.wrap         = bool(wrap)
        if self.wrap:
            # Each bit responds to a range of input values, length of range is 2*Radius.
            radius        = self.len_drange * self.sparsity / 2
            self.offsets  = np.random.uniform(0, self.len_drange, self.input_shape)
            self.offsets  = np.array(self.offsets, dtype=self.dtype)
            # If wrapping is enabled then don't generate ranges which will be
            # truncated near the edges.
            centers = np.random.uniform(min(self.drange) + radius,
                                        max(self.drange) - radius,
                                        size=self.dimensions)
        else:
            # Buckets near the edges of the datarange are OK.  They will not
            # respond to a full range of input values but are needed to
            # represent the bits at the edges of the data range.
            #
            # Expand the data range and create bits which will encode for the
            # edges of the datarange to ensure a resonable sparsity at the
            # extremes of the data ragne.  Increase the size of all of the
            # buckets to accomidate for the extra area being represented.
            M = .95 # Maximum fraction of bucket-size outside of the true data range to allocate buckets.
            len_pad_drange = self.len_drange / (1 - M * self.sparsity)
            extra_space    = (len_pad_drange - self.len_drange) / 2
            pad_drange     = (min(self.drange) - extra_space, max(self.drange) + extra_space)
            radius         = len_pad_drange * self.sparsity / 2
            centers = np.random.uniform(min(pad_drange),
                                        max(pad_drange),
                                        size=self.dimensions)
        # Make the lower and upper bounds of the ranges.
        low  = np.clip(centers - radius, min(self.drange), max(self.drange))
        high = np.clip(centers + radius, min(self.drange), max(self.drange))
        self.low  = np.array(low, dtype=self.dtype)
        self.high = np.array(high, dtype=self.dtype)

    def encode(self, img):
        """
        Argument img - ndarray
        Returns a SDR
        """
        #assert(isinstance(inp, SDR)) #TODO make Channel take SDR as input too
        #img = np.ndarray(inp.dense).reshape(self.input_shape)
        assert(img.shape == self.input_shape)
        assert(img.dtype == self.dtype)
        if self.wrap:
            img += self.offsets
            # Technically this should subtract min(drange) before doing modulus
            # but the results should also be indistinguishable B/C of the random
            # offsets.  Min(drange) effectively becomes part of the offset.
            img %= self.len_drange
            img += min(self.drange)
        img = img.reshape(img.shape + (1,))
        img = np.logical_and(self.low <= img, img <= self.high)
        enc = SDR(self.dimensions)
        enc.dense = img
        return enc


class Eye:
    """
    Optic sensor with central fovae.
    Simulates functionality of eye's retinal parvocellular(P-cells),
    and magnocellular(M-cells) pathways, at the saccadic steps. 

    On high level, 
    magno cells: 
      - detect change in temporal information in the image, ie motion 
        detection, video processing tasks, ... 
    parvo cells: 
      - detect color, shape information in the (static) image. Useful
        for image classification, etc. 
    For more details see: 
    https://foundationsofvision.stanford.edu/chapter-5-the-retinal-representation/#visualinformation

   #TODO the motion-control of "where to look at" is not fully researched 
   and covered by this code. You need to manually control the positions 
   of the eye/sensor (where it looks at) at the level of saccades. 
    

    Attribute roi ... The most recent view, kept as a attribute.
    Attribute parvo_sdr ... SDR with parvocellular pathway (color)
    Attribute magno_sdr ... SDR with magnocellular pathway (movement) 

    The following three attributes control where the eye is looking within
    the image.  They are Read/Writable.
      Note: (X,Y,scale,rotation) require manual control by the user, as in the brain
      this is part of the Motor-Control, which is not in the scope of this encoder. 

    Attribute position     (X, Y) coords of eye center within image
      (wrt [0,0] corner of the image). Default starting position is the center
      of the image. 
    Attribute orientation  ... units are radians, the rotation of the sensor
      (wrt the image)
    Attribute scale        ... The scale controls the distance between the eye
      and the image (scale is similar to distance on Z-axis). 
      Note: In experiments you typically need to manually tune the `scale` for the dataset, 
      to find a reasonable value through trial and error.
      More explanation: "When you look at a really big image, you need to take
      a few steps back to be able to see the whole thing. The scale parameter 
      allows the eye to do just that, walk forwards and backwards from the image,
      to find good place to view it from."
    """


    def __init__(self,
        output_diameter   = 200, # output SDR size is diameter^2
        sparsityParvo     = 0.2,
        sparsityMagno     = 0.025,
        color             = True,):
        """
        Argument output_diameter is size of output ... output is a 
            field of view (image) with circular shape. Default 200. 
            `parvo/magno_sdr` size is `output_diameter^2`
        Argument `sparsityParvo` - sparsity of parvo-cellular pathway of the eye.
            As a simplification, "parvo" cells (P-cells) represent colors, static 
            object's properties (shape,...) and are used for image classification. 
            For biologically accurate details see eg. 
            https://foundationsofvision.stanford.edu/chapter-5-the-retinal-representation/
            Note: biologically, the ratio between P/M-cells is about 8(P):1(M):(1 rest), see
            https://www.pnas.org/content/94/11/5900
        Argument `sparsityMagno` - sparsity of the magno-cellular (M-cells) pathway, which
            transfers (higly) temporal information in the visual data, as use "used" for
            motion detection and motion tracking, video processing.
            For details see @param `sparsityParvo`.
            TODO: output of M-cells should be processed on a fast TM.
        Argument color: use color vision (requires P-cells > 0), default true.
        """
        self.output_diameter   = output_diameter
        # Argument resolution_factor is used to expand the sensor array so that
        # the fovea has adequate resolution.  After log-polar transform image
        # is reduced by this factor back to the output_diameter.
        self.resolution_factor = 2
        self.retina_diameter   = int(self.resolution_factor * output_diameter)
        # Argument fovea_scale  ... proportion of the image (ROI) which will be covered (seen) by
        # high-res fovea (parvo pathway)
        self.fovea_scale       = 0.177
        assert(output_diameter % 2 == 0) # Diameter must be an even number.
        assert(self.retina_diameter % 2 == 0) # (Resolution Factor X Diameter) must be an even number.
        assert(sparsityParvo >= 0 and sparsityParvo <= 1.0)
        if sparsityParvo > 0:
          assert(sparsityParvo * (self.retina_diameter **2) > 0)
        self.sparsityParvo = sparsityParvo
        assert(sparsityMagno >= 0 and sparsityMagno <= 1.0)
        if sparsityMagno > 0:
          assert(sparsityMagno * (self.retina_diameter **2) > 0)
        self.sparsityMagno = sparsityMagno
        if color is True:
          assert(sparsityParvo > 0)
        self.color = color


        self.retina = cv2.bioinspired.Retina_create(
            inputSize            = (self.retina_diameter, self.retina_diameter),
            colorMode            = color,
            colorSamplingMethod  = cv2.bioinspired.RETINA_COLOR_BAYER,
            useRetinaLogSampling = True,)

        print(self.retina.printSetup())
        print()

        if sparsityParvo > 0:
          dims = (output_diameter, output_diameter)

          sparsityP_ = sparsityParvo
          if color is True: 
            dims = (output_diameter, output_diameter, 3,) #3 for RGB color channels

            # The reason the parvo-cellular has `3rd-root of the sparsity` is that there are three color channels (RGB), 
            # each of which is encoded separately and then combined. The color channels are combined with a logical AND, 
            # which on average reduces the sparsity.
            # This same principal applies to any time you combine two encoders with a logical bit-wise AND, the final combined
            # sparsity would be `sparsity^n`, hence the cubic root for 3 dims: 
            sparsityP_ = sparsityParvo ** (1/3.)

          self.parvo_enc = ChannelEncoder(
                            input_shape = dims,
                            num_samples = 1, 
                            sparsity = sparsityP_,
                            dtype=np.uint8, drange=[0, 255,])
        else:
          self.parvo_enc = None

        if sparsityMagno > 0:
          self.magno_enc = ChannelEncoder(
                            input_shape = (output_diameter, output_diameter),
                            num_samples = 1, 
                            sparsity = sparsityMagno,
                            dtype=np.uint8, drange=[0, 255],)
        else:
          self.magno_enc = None

        ## output variables:
        self.dimensions = (output_diameter, output_diameter,)
        self.size       = np.prod(self.dimensions)
        self.image      = None # the current input RGB image
        self.roi        = np.zeros(self.dimensions) # self.image cropped to region of interest, what encoder processes ("sees")
        self.parvo_img  = None # output visualization of parvo/magno cells
        self.magno_img  = None
        self.parvo_sdr  = SDR(self.dimensions) # parvo/magno cellular representation (SDR)
        self.magno_sdr  = SDR(self.dimensions)

        ## motor-control variables (must be user specified)
        self.position   = (0,0) # can use self.center_view(), self.random_view()
        self.scale      = 1.0 # represents "zoom" aka distance from the object/image 
        self.orientation= 0 # angle between image/object and camera, in deg


    def _new_image(self, image):
        """
        Argument image ...
            If String, will load image from file path.
            If numpy.ndarray, will attempt to cast to correct data type and
                dimensions.

        For demo, run: 
        python py/htm/encoders/eye.py py/tests/encoders/ronja_the_cat.jpg
        """
        # Load image if needed.
        if isinstance(image, str):
            self.image = cv2.imread(image)
            self.image = cv2.cvtColor(self.image, cv2.COLOR_BGR2RGB)
        else:
            self.image = image
        # Get the image into the right format.
        assert(isinstance(self.image, np.ndarray))
        # Ensure there are three color channels.
        if len(self.image.shape) == 2 or self.image.shape[2] == 1:
            self.image = np.dstack([self.image] * 3)
        # Drop the alpha channel if present.
        elif self.image.shape[2] == 4:
            self.image = self.image[:,:,:3]
        # Sanity checks.
        assert(len(self.image.shape) == 3)
        assert(self.image.shape[2] == 3) # Color images only.
        self.reset()
        self.center_view()
        self.roi = None
        assert(min(self.image.shape[:2]) >= self.retina_diameter)
        return self.image


    def center_view(self):
        """Center the view over the image"""
        self.orientation = 0
        self.position    = (self.image.shape[0]/2., self.image.shape[1]/2.)
        self.scale       = np.min(np.divide(self.image.shape[:2], self.retina_diameter))
        self.roi = None #changing center breaks prev ROI

    def randomize_view(self, scale_range=None):
        """Set the eye's view point to a random location"""
        if scale_range is None:
            scale_range = [2, min(self.image.shape[:2]) / self.retina_diameter]
        assert(len(scale_range) == 2)
        self.orientation = random.uniform(0, 2 * math.pi)
        self.scale       = random.uniform(min(scale_range), max(scale_range))
        roi_radius       = self.scale * self.retina_diameter / 2
        self.position    = [random.uniform(roi_radius, dim - roi_radius)
                                 for dim in self.image.shape[:2]]
        self.roi = None


    def _crop_roi(image, position, diameter, scale):
        """
        Crop to Region Of Interest (ROI) which contains the whole field of view.
        Adds a black circular boarder to mask out areas which the eye can't see.

        Note: size of the ROI is (eye.output_diameter * eye.resolution_factor).
        Note: the circular boarder is actually a bit too far out, playing with
          eye.fovea_scale can hide areas which this ROI image will show.

        Arguments: eye.scale, eye.position, eye.image

        Returns RGB image (diameter * diameter, but effectively cropped to an 
          inner circle - FOV). 

        See also, @see make_roi_pretty()
        """
        assert(isinstance(image, np.ndarray))
        assert(diameter > 0)
        assert(scale > 0)

        r     = int(round(scale * diameter / 2))
        assert(r > 0)
        x, y  = position
        x     = int(round(x))
        y     = int(round(y))
        x_max, y_max, color_depth = image.shape

        # Find the boundary of the ROI and slice out the image.
        x_low  = max(0, x-r)
        x_high = min(x_max, x+r)
        y_low  = max(0, y-r)
        y_high = min(y_max, y+r)
        image_slice = image[x_low : x_high, y_low : y_high]

        # Make the ROI and insert the image into it.
        roi = np.zeros((2*r, 2*r, 3,), dtype=np.uint8)
        if x-r < 0:
            x_offset = abs(x-r)
        else:
            x_offset = 0
        if y-r < 0:
            y_offset = abs(y-r)
        else:
            y_offset = 0
        x_shape, y_shape, color_depth = image_slice.shape
        roi[x_offset:x_offset+x_shape, y_offset:y_offset+y_shape] = image_slice

        # Rescale the ROI to remove the scaling effect.
        roi = cv2.resize(roi, (diameter, diameter),  interpolation = cv2.INTER_AREA)

        # Mask out areas the eye can't see by drawing a circle boarder.
        # this represents the "shape" of the sensor/eye (comment out to leave rectangural)
        center = int(roi.shape[0] / 2)
        circle_mask = np.zeros(roi.shape, dtype=np.uint8)
        cv2.circle(circle_mask, (center, center), center, thickness = -1, color=(255,255,255))
        roi = np.minimum(roi, circle_mask)

        return roi


    def compute(self, img, position=None, rotation=None, scale=None):
        """
        Argument img - image to load. String/data, see _new_image()
        Arguments position, rotation, scale: optional, if not None, the self.xxx is overriden
          with the provided value.
        Returns tuple (SDR parvo, SDR magno) 
        """
        self.image = self._new_image(img)
        assert(self.image is not None)

        # set position
        if position is not None:
          self.position = position
        if rotation is not None:
          self.orientation=rotation
        if scale is not None:
          self.scale=scale

        # apply field of view (FOV), rotation
        self.roi = self.rotate_(self.image, self.orientation) 
        self.roi = Eye._crop_roi(self.roi, self.position, self.retina_diameter, self.scale)

        # Retina image transforms (Parvo & Magnocellular).
        self.retina.run(self.roi)
        if self.parvo_enc is not None:
          parvo = self.retina.getParvo()
        if self.magno_enc is not None:
          magno = self.retina.getMagno()


        # Log Polar Transform.
        center = self.retina_diameter / 2
        M      = self.retina_diameter * self.fovea_scale
        M = min(M, self.output_diameter)
        if self.parvo_enc is not None:
          parvo = cv2.logPolar(parvo,
                               center = (center, center),
                               M = M,
                               flags = cv2.WARP_FILL_OUTLIERS)
          parvo = cv2.resize(parvo,  dsize=(self.output_diameter, self.output_diameter), interpolation = cv2.INTER_CUBIC)
          self.parvo_img = parvo

        if self.magno_enc is not None:
          magno = cv2.logPolar(magno,
                               center = (center, center),
                               M = M,
                               flags = cv2.WARP_FILL_OUTLIERS)
          magno = cv2.resize(magno, dsize=(self.output_diameter, self.output_diameter), interpolation = cv2.INTER_CUBIC)
          self.magno_img = magno

        # Encode images into SDRs.
        if self.parvo_enc is not None:
          p   = self.parvo_enc.encode(parvo).dense
          if self.color:
            pr, pg, pb = np.dsplit(p, 3)
            p   = np.logical_and(np.logical_and(pr, pg), pb)
          p   = np.expand_dims(np.squeeze(p), axis=2)
          self.parvo_sdr.dense = p.flatten()
          assert(len(self.parvo_sdr.sparse) > 0)

        if self.magno_enc is not None:
          self.magno_sdr = self.magno_enc.encode(magno)
          assert(len(self.magno_sdr.sparse) > 0)

        return (self.parvo_sdr, self.magno_sdr)


    def _make_roi_pretty(self, roi):
        """
        Makes the eye's view look more presentable.
        - Adds 5 dots to the center of the image to show where the fovea is.

        Returns an RGB image.
        See _crop_roi()
        """
        assert(roi is not None)

        # Invert 5 pixels in the center to show where the fovea is located.
        center = int(roi.shape[0] / 2)
        roi[center, center]     = np.full(3, 255) - roi[center, center]
        roi[center+2, center+2] = np.full(3, 255) - roi[center+2, center+2]
        roi[center-2, center+2] = np.full(3, 255) - roi[center-2, center+2]
        roi[center-2, center-2] = np.full(3, 255) - roi[center-2, center-2]
        roi[center+2, center-2] = np.full(3, 255) - roi[center+2, center-2]

        # Draw a red circle where fovea (=high resolution parvocellular vision) boundary is
        cv2.circle(roi, (center, center), radius=int(self.retina_diameter*self.fovea_scale), color=(255,0,0), thickness=3)

        return roi


    def rotate_(self, img, angle):
      """
      rotate the image img, by angle in degrees
      """
      assert(isinstance(img, np.ndarray))
      angle = angle * 360 / (2 * math.pi)
      rows, cols, color_depth = img.shape
      M   = cv2.getRotationMatrix2D((cols / 2, rows / 2), angle, 1)
      return cv2.warpAffine(img, M, (cols,rows))


    def plot(self, window_name='Eye', delay=1000):
        roi = self._make_roi_pretty(self.roi)
        cv2.imshow('Region Of Interest', roi)
        cv2.imshow('Whole image', self.image)

        if self.sparsityParvo > 0: # parvo enabled
          if self.color:
            cv2.imshow('Parvocellular', self.parvo_img[:,:,::-1])
            cv2.imshow('Parvo retinal representation', self.retina.getParvo()[:,:,::-1])
          else:
            cv2.imshow('Parvocellular', self.parvo_img)
          idx = self.parvo_sdr.dense.astype(np.uint8).reshape(self.output_diameter, self.output_diameter)*255
          cv2.imshow('Parvo SDR', idx)

        if self.sparsityMagno > 0: # magno enabled
          cv2.imshow('Magnocellular', self.magno_img)
          idx = self.magno_sdr.dense.astype(np.uint8).reshape(self.output_diameter, self.output_diameter)*255
          cv2.imshow('Magno SDR', idx)

        cv2.waitKey(delay)


    def small_random_movement(self):
        """returns small difference in position, rotation, scale.
           This is naive "saccadic" movements.
        """
        max_change_angle = (2*math.pi) / 100
        self.position = (
            self.position[0] + random.gauss(1.2, .75),
            self.position[1] + random.gauss(1.2, .75),)
        self.orientation += random.uniform(-max_change_angle, max_change_angle)
        self.scale += random.gauss(0, 0.1)
        return (self.position, self.orientation, self.scale)


    def reset(self):
        self.retina.clearBuffers()




def _get_images(path):
    """ Returns list of all image files found under the given file path. """
    image_extensions = [
        '.bmp',
        '.dib',
        '.png',
        '.jpg',
        '.jpeg',
        '.jpe',
        '.tif',
        '.tiff',
    ]
    images = []
    import os
    if os.path.isfile(path):
        basename, ext = os.path.splitext(path)
        if ext.lower() in image_extensions:
            images.append( path )
    else:
        for dirpath, dirnames, filenames in os.walk(path):
            for fn in filenames:
                basename, ext = os.path.splitext(fn)
                if ext.lower() in image_extensions:
                    images.append( os.path.join(dirpath, fn) )
    return images

if __name__ == '__main__':
    import argparse
    args = argparse.ArgumentParser()
    args.add_argument('IMAGE', type=str)
    args   = args.parse_args()
    images = _get_images( args.IMAGE )
    random.shuffle(images)
    if not images:
        print('No images found at file path "%s"!'%args.IMAGE)
    else:
        eye = Eye(output_diameter=200,
                  sparsityParvo=0.2,
                  sparsityMagno=0.02,
                  color=True)
        for img_path in images:
            eye.reset()
            eye.fovea_scale = 0.077 #TODO find which value?
            #eye.center_view()
            eye.position=(400,400)
            for i in range(10):
                pos,rot,sc = eye.small_random_movement()
                (sdrParvo, sdrMagno) = eye.compute(img_path, pos,rot,sc) #TODO derive from Encoder
                eye.plot(delay=5000)
            print("Sparsity parvo: {}".format(len(eye.parvo_sdr.sparse)/np.product(eye.dimensions)))
            print("Sparsity magno: {}".format(len(eye.magno_sdr.sparse)/np.product(eye.dimensions)))
        print("All images seen.")
