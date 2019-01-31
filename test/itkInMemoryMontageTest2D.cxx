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

#include "itkInMemoryMontageTestHelper.hxx"
#include "itkParseTileConfiguration.h"
#include "itkRGBPixel.h"

enum class TestVariation : unsigned int
{
  UOrigin_USpacing_UTransform = 0,
  UOrigin_DSpacing_UTransform,
  UOrigin_USpacing_DTransform,
  UOrigin_DSpacing_DTransform,
  DOrigin_USpacing_UTransform,
  DOrigin_DSpacing_UTransform,
  DOrigin_USpacing_DTransform,
  DOrigin_DSpacing_DTransform,
};

int itkInMemoryMontageTest2D(int argc, char* argv[])
{
  if ( argc < 3 )
    {
    std::cerr << "Usage: " << argv[0] << " <directoryWithInputData> <montageTSV>";
    std::cerr << " [ variation streamSubdivisions ]" << std::endl;
    return EXIT_FAILURE;
    }

  TestVariation variation = TestVariation::UOrigin_USpacing_UTransform;
  if ( argc > 3 )
    {
    variation = static_cast<TestVariation>(std::stoul( argv[3] ));
    }

  unsigned streamSubdivisions = 1;
  if ( argc > 4 )
    {
    streamSubdivisions = std::stoul( argv[4] );
    }

  std::string inputPath = argv[1];
  if ( inputPath.back() != '/' && inputPath.back() != '\\' )
    {
    inputPath += '/';
    }

  itk::TileLayout2D stageTiles = itk::ParseTileConfiguration2D( inputPath + "TileConfiguration.txt" );

  itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(
    ( inputPath + stageTiles[0][0].FileName ).c_str(), itk::ImageIOFactory::ReadMode );
  imageIO->SetFileName( inputPath + stageTiles[0][0].FileName );
  imageIO->ReadImageInformation();
  const itk::ImageIOBase::IOPixelType pixelType = imageIO->GetPixelType();

  std::string outFileName = std::string(argv[2]);

  int r1;
  if (pixelType == itk::ImageIOBase::IOPixelType::RGB)
    {
    return inMemoryMontageTest< itk::RGBPixel< unsigned char >, itk::RGBPixel< unsigned int >, TestVariation >(
      stageTiles, inputPath, outFileName, variation, streamSubdivisions );
    }
  else
    {
    return inMemoryMontageTest< unsigned short, double, TestVariation >(
      stageTiles, inputPath, outFileName, variation, streamSubdivisions );
    }
}
