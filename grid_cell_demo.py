#!/usr/bin/python3
# Reproduction study of the (Kropff & Treves, 2008) grid cell model.
# Written by David McDougall, 2019
import argparse
import numpy as np
import random
import itertools
import math
import scipy.signal
import time
import matplotlib.pyplot as plt

from nupic.bindings.sdr import SDR, Metrics
from nupic.bindings.encoders import CoordinateEncoder, CoordinateEncoderParameters
from nupic.bindings.algorithms import ColumnPooler, NoTopology


class Environment(object):
  """
  Environment is a 2D square, in first quadrant with corner at origin.
  """
  def __init__(self, size):
    self.size     = size
    self.position = (size/2, size/2)
    self.speed    = 2.0 ** .5
    self.angle    = 0
    self.course   = []

  def in_bounds(self, position):
    x, y = position
    x_in = x >= 0 and x < self.size
    y_in = y >= 0 and y < self.size
    return x_in and y_in

  def move(self):
    max_rotation = 2 * math.pi / 20
    self.angle += random.uniform(-max_rotation, max_rotation)
    vx = self.speed * math.cos(self.angle)
    vy = self.speed * math.sin(self.angle)
    x, y = self.position
    new_position = (x + vx, y + vy)
    
    if self.in_bounds(new_position):
      self.position = new_position
      self.course.append(self.position)
    else:
      # On failure, recurse and try again.
      assert(self.in_bounds(self.position))
      self.angle = random.uniform(0, 2 * math.pi)
      self.move()

  def plot_course(self, show=True):
    plt.figure("Path")
    plt.ylim([0, self.size])
    plt.xlim([0, self.size])
    x, y = zip(*self.course)
    plt.plot(x, y, 'k-')
    if show:
      plt.show()


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument('--train_time', type=int, default = 1000 * 1000,)
  parser.add_argument('--debug', action='store_true')
  args = parser.parse_args()

  # Setup
  env = Environment(size = 200)

  enc = CoordinateEncoderParameters()
  enc.numDimensions = 2
  enc.size          = 2500
  enc.activeBits    = 75
  # enc.radius        = 5  # TODO Radius unimplemented
  enc.resolution    = 1 # TODO: Find an equivalent for radius 5
  enc = CoordinateEncoder(enc)
  assert(enc.parameters.activeBits > 50)

  gcm = ColumnPooler.defaultParameters
  gcm.stability_rate              = 1 - 0.05
  gcm.fatigue_rate                = 0.05 / 3
  gcm.proximalInputDimensions     = (enc.size,)
  gcm.inhibitionDimensions        = (1,)
  gcm.cellsPerInhibitionArea      = 100
  gcm.proximalSynapseThreshold    = 0.25
  gcm.sparsity                    = .2
  gcm.proximalIncrement           = 0.005
  gcm.proximalDecrement           = 0.0008
  gcm.proximalSegments            = 1
  gcm.proximalSegmentThreshold    = 0
  gcm.period                      = 10000
  gcm.potentialPool               = NoTopology( 0.95 )
  gcm.distalInputDimensions       = (0,)
  gcm.verbose                     = True
  gcm = ColumnPooler(gcm)

  def compute(learn=True):
    enc_sdr = enc.encode(env.position)
    gc_act  = SDR( gcm.cellDimensions )
    extra   = SDR( 0 )
    gcm.compute(enc_sdr, extra, learn, gc_act)
    return enc_sdr, gc_act


  print("Training for %d cycles ..."%args.train_time)
  start_time = time.time()
  gcm.reset()
  gc_metrics = Metrics(gcm.cellDimensions, args.train_time)
  for step in range(args.train_time):
    if step % 10000 == 0:
      print("Cycle %d"%step)
    env.move()
    enc_sdr, gc_sdr = compute()
    gc_metrics.addData( gc_sdr )
  train_time = time.time()
  print("Elapsed time (training): %d seconds."%int(round(train_time - start_time)))
  print("Grid Cell Metrics", str(gc_metrics))

  print("Testing ...")

  # Show how the agent traversed the environment.
  if args.train_time <= 100000:
    env.plot_course(show=False)
  else:
    print("Not going to plot course, too long.")

  # Measure Receptive Fields.
  enc_num_samples = 12
  gc_num_samples  = 20
  enc_samples = random.sample(range(enc.parameters.size), enc_num_samples)
  gc_samples  = random.sample(range(np.product(gcm.cellDimensions)), gc_num_samples)
  enc_rfs = [np.zeros((env.size, env.size)) for idx in enc_samples]
  gc_rfs  = [np.zeros((env.size, env.size)) for idx in gc_samples]
  for position in itertools.product(range(env.size), range(env.size)):
    env.position = position
    gcm.reset()
    enc_sdr, gc_sdr = compute(learn=False)
    for rf_idx, enc_idx in enumerate(enc_samples):
      enc_rfs[rf_idx][position] = enc_sdr.dense[enc_idx]
    for rf_idx, gc_idx in enumerate(gc_samples):
      gc_rfs[rf_idx][position] = gc_sdr.dense.flatten()[gc_idx]

  # Show the Input/Encoder Receptive Fields.
  if enc_num_samples > 0:
    plt.figure("Input Receptive Fields")
    nrows = int(enc_num_samples ** .5)
    ncols = math.ceil((enc_num_samples+.0) / nrows)
    for subplot_idx, rf in enumerate(enc_rfs):
      plt.subplot(nrows, ncols, subplot_idx + 1)
      plt.imshow(rf, interpolation='nearest')

  # Show the Grid Cells Receptive Fields.
  if gc_num_samples > 0:
    plt.figure("Grid Cell Receptive Fields")
    nrows = int(gc_num_samples ** .5)
    ncols = math.ceil((gc_num_samples+.0) / nrows)
    for subplot_idx, rf in enumerate(gc_rfs):
      plt.subplot(nrows, ncols, subplot_idx + 1)
      plt.imshow(rf, interpolation='nearest')

    if not args.debug:
      # Show the autocorrelations of the grid cell receptive fields.
      plt.figure("Grid Cell RF Autocorrelations")
      for subplot_idx, rf in enumerate(gc_rfs):
        plt.subplot(nrows, ncols, subplot_idx + 1)
        xcor = scipy.signal.correlate2d(rf, rf)
        plt.imshow(xcor, interpolation='nearest')

  test_time = time.time()
  print("Elapsed time (testing): %d seconds."%int(round(test_time - train_time)))
  plt.show()
