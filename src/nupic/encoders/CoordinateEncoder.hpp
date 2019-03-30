/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2019, David McDougall
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * --------------------------------------------------------------------- */

/** @file
 * Define the CoordinateEncoder & CoordinateEncoderParameters
 */

#ifndef NTA_ENCODERS_COORD
#define NTA_ENCODERS_COORD

#include <nupic/types/Sdr.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/utils/Random.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/encoders/RandomDistributedScalarEncoder.hpp>

namespace nupic {
namespace encoders {

/*
 * TODO: DOCUMENTATION
 */
struct CoordinateEncoderParameters : public RDSE_Parameters {
  /*
   * TODO: DOCUMENTATION
   */
  UInt numDimensions;
};

/*
 * TODO: DOCUMENTATION
 */
class CoordinateEncoder : public BaseEncoder<const std::vector<Real64> &>
{
public:
  CoordinateEncoder() {}
  CoordinateEncoder( const CoordinateEncoderParameters &parameters );
  void initialize( const CoordinateEncoderParameters &parameters );

  const CoordinateEncoderParameters &parameters = args_;

  void encode(const std::vector<Real64> &coordinates, sdr::SDR &output) override;

  void save(std::ostream& ) const override {}; // TODO: IMPLEMENTATION
  void load(std::istream& ) override {}; // TODO: IMPLEMENTATION

  ~CoordinateEncoder() override {};

private:
  CoordinateEncoderParameters args_;
  sdr::SDR neighborhood_;
};     // End class CoordinateEncoder
}      // End namespace encoders
}      // End namespace nupic
#endif // End ifdef NTA_ENCODERS_COORD
