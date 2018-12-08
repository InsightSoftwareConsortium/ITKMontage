/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef itkParseTileConfiguration_h
#define itkParseTileConfiguration_h

#include "MontageExport.h"

#include <string>
#include <vector>

#include "itkPoint.h"

namespace itk
{
template< unsigned Dimension >
struct Tile
{
  itk::Point< double, Dimension > Position; // x, y... coordinates

  std::string FileName;
};

Montage_EXPORT std::vector< std::vector< Tile< 2 > > >
ParseTileConfiguration2D( const std::string pathToFile );

} // namespace itk

#endif // itkParseTileConfiguration_h
