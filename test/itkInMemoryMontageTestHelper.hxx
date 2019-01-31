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

#ifndef itkMontageTestHelper_hxx
#define itkMontageTestHelper_hxx

#include "itkAffineTransform.h"
#include "itkImageFileWriter.h"
#include "itkMaxPhaseCorrelationOptimizer.h"
#include "itkParseTileConfiguration.h"
#include "itkSimpleFilterWatcher.h"
#include "itkTileMergeImageFilter.h"
#include "itkTileMontage.h"
#include "itkTransformFileWriter.h"
#include "itkTxtTransformIOFactory.h"

#include <fstream>
#include <iomanip>
#include <type_traits>

template< typename TransformType >
void
WriteTransform( const TransformType* transform, std::string filename )
{
  using AffineType = itk::AffineTransform< double, 3 >;
  using TransformWriterType = itk::TransformFileWriterTemplate< double >;
  TransformWriterType::Pointer tWriter = TransformWriterType::New();
  tWriter->SetFileName( filename );

  if ( TransformType::SpaceDimension >= 2 || TransformType::SpaceDimension <= 3 )
    { // convert into affine which Slicer can read
    AffineType::Pointer aTr = AffineType::New();
    AffineType::TranslationType t;
    t.Fill( 0 );
    for ( unsigned i = 0; i < TransformType::SpaceDimension; i++ )
      {
      t[i] = transform->GetOffset()[i];
      }
    aTr->SetTranslation( t );
    tWriter->SetInput( aTr );
    }
  else
    {
    tWriter->SetInput( transform );
    }
  tWriter->Update();
}

template< typename TImage >
typename TImage::Pointer
ReadImage( const char* filename )
{
  using ReaderType = itk::ImageFileReader< TImage >;
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( filename );
  reader->Update();
  return reader->GetOutput();
}

template< typename Origin2D, typename OriginRow, typename OriginPoint, unsigned int Dim >
Origin2D createUnitOrigin2D(int numOfRows, int numOfCols)
{
  Origin2D UO;

  OriginPoint UO_point;
  for (int i = 0; i < Dim; i++)
  {
    UO_point[i] = 0;
  }

  for (int i = 0; i < numOfRows; i++)
  {
    OriginRow row;
    for (int i = 0; i < numOfCols; i++)
    {
      row.push_back(UO_point);
    }
    UO.push_back(row);
  }

  return UO;
}

template< typename Origin2D, typename OriginRow, typename OriginPoint, typename StageTiles,
          typename TileRow2D, typename Tile2D >
Origin2D createOrigin2DFromStageTiles(StageTiles stageTiles)
{
  Origin2D UO;

  for (TileRow2D tileRow : stageTiles)
  {
    OriginRow row;
    for (Tile2D tile : tileRow)
    {
      row.push_back(tile.Position);
    }
    UO.push_back(row);
  }

  return UO;
}

template< typename Spacing2D, typename SpacingRow, typename SpacingType, unsigned int Dim >
Spacing2D createSpacing2D(int numOfRows, int numOfCols, double value)
{
  Spacing2D US;

  SpacingType spacing;
  for (int i = 0; i < Dim; i++)
  {
    spacing[i] = value;
  }

  for (int i = 0; i < numOfRows; i++)
  {
    SpacingRow row;
    for (int i = 0; i < numOfCols; i++)
    {
      row.push_back(spacing);
    }
    US.push_back(row);
  }

  return US;
}

template< typename Transform2D, typename TransformPtrRow, typename MontageType >
Transform2D createUnitTransform2D(int numOfRows, int numOfCols)
{
  Transform2D UT;

  for (int i = 0; i < numOfRows; i++)
  {
    TransformPtrRow transform_row;
    for (int i = 0; i < numOfCols; i++)
    {
      typename MontageType::TransformPointer transform = MontageType::TransformType::New();
      transform_row.push_back(transform);
    }
    UT.push_back(transform_row);
  }

  return UT;
}

template< typename Transform2D, typename TransformPtrRow, typename MontageType, typename StageTiles,
          typename TileRow2D, typename Tile2D >
Transform2D createTransform2DFromStageTiles(StageTiles stageTiles)
{
  Transform2D UT;

  for (TileRow2D tileRow : stageTiles)
  {
    TransformPtrRow transform_row;
    for (Tile2D tile : tileRow)
    {
      typename MontageType::TransformPointer transform = MontageType::TransformType::New();

      auto offset = transform->GetOffset();
      for (unsigned i = 0; i < MontageType::TransformType::SpaceDimension; i++)
      {
        offset[i] = tile.Position[i];
      }

      transform->SetOffset(offset);

      transform_row.push_back(transform);
    }
    UT.push_back(transform_row);
  }

  return UT;
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that executes a given montage test with a given origin2D, spacing2D, and transform2D
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType, typename Origin2D,
          typename Spacing2D, typename Transform2D >
int
executeInMemoryMontageTest( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, Origin2D origin2D, Spacing2D spacing2D,
                            Transform2D transform2D, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;

  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using PointType = itk::Point< double, Dimension >;
  using VectorType = itk::Vector< double, Dimension >;
  using TransformType = itk::TranslationTransform< double, Dimension >;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using OriginalImageType = itk::Image< PixelType, Dimension >; // possibly RGB instead of scalar
  using PCMType = itk::PhaseCorrelationImageRegistrationMethod< ScalarImageType, ScalarImageType >;
  using PadMethodUnderlying = typename std::underlying_type< typename PCMType::PaddingMethod >::type;
  using MontageType = itk::TileMontage< ScalarImageType >;

  itk::ObjectFactoryBase::RegisterFactory( itk::TxtTransformIOFactory::New() );
  unsigned yMontageSize = origin2D.size();
  unsigned xMontageSize = origin2D[0].size();

  // write generated mosaic
  typename MontageType::TileIndexType ind;
  using Resampler = itk::TileMergeImageFilter< OriginalImageType, AccumulatePixelType >;
  typename Resampler::Pointer resampleF = Resampler::New();
  itk::SimpleFilterWatcher fw2( resampleF, "resampler" );
  resampleF->SetMontageSize( { xMontageSize, yMontageSize } );
  for ( unsigned y = 0; y < yMontageSize; y++ )
    {
    ind[1] = y;
    for ( unsigned x = 0; x < xMontageSize; x++ )
      {
      ind[0] = x;
      std::string filename = inputPath + stageTiles[y][x].FileName;

      typename OriginalImageType::Pointer image = ReadImage< typename Resampler::ImageType >( filename.c_str() );
      typename OriginalImageType::PointType origin = origin2D[y][x];
      typename OriginalImageType::SpacingType spacing = spacing2D[y][x];
      image->SetOrigin(origin);
      image->SetSpacing(spacing);
      resampleF->SetInputTile( ind, image );

      typename MontageType::TransformConstPointer transform = transform2D[y][x];
      resampleF->SetTileTransform( ind, transform );
      }
    }

  resampleF->Update();

  using WriterType = itk::ImageFileWriter< OriginalImageType >;
  typename WriterType::Pointer w = WriterType::New();
  w->SetInput( resampleF->GetOutput() );
  // resampleF->DebugOn(); //generate an image of contributing regions
  // MetaImage format supports streaming
  w->SetFileName( outFilename );
  // w->UseCompressionOn();
  w->SetNumberOfStreamDivisions( streamSubdivisions );
  w->Update();

  return EXIT_SUCCESS;
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a unit origin, unit spacing, and unit transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest0( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D UO = createUnitOrigin2D<Origin2D,OriginRow,OriginPoint,Dimension>(yMontageSize, xMontageSize);

  Spacing2D US = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 1);

  Transform2D UT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, UO, US, UT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a unit origin, dynamic spacing, and unit transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest1( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D UO = createUnitOrigin2D<Origin2D,OriginRow,OriginPoint,Dimension>(yMontageSize, xMontageSize);

  Spacing2D DS = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 0.5);

  Transform2D UT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, UO, DS, UT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a unit origin, unit spacing, and dynamic transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest2( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D UO = createUnitOrigin2D<Origin2D,OriginRow,OriginPoint,Dimension>(yMontageSize, xMontageSize);

  Spacing2D US = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 1);

  Transform2D DT = createTransform2DFromStageTiles<Transform2D,TransformPtrRow,MontageType,itk::TileLayout2D,
      itk::TileRow2D,itk::Tile2D>(stageTiles);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, UO, US, DT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a unit origin, dynamic spacing, and dynamic transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest3( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D UO = createUnitOrigin2D<Origin2D,OriginRow,OriginPoint,Dimension>(yMontageSize, xMontageSize);

  Spacing2D DS = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 0.5);

  Transform2D DT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, UO, DS, DT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a dynamic origin, unit spacing, and unit transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest4( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D DO = createOrigin2DFromStageTiles<Origin2D,OriginRow,OriginPoint,
      itk::TileLayout2D,itk::TileRow2D,itk::Tile2D>(stageTiles);

  Spacing2D US = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 1);

  Transform2D UT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, DO, US, UT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a dynamic origin, dynamic spacing, and unit transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest5( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D DO = createOrigin2DFromStageTiles<Origin2D,OriginRow,OriginPoint,
      itk::TileLayout2D,itk::TileRow2D,itk::Tile2D>(stageTiles);

  Spacing2D DS = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 0.5);

  Transform2D UT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, DO, DS, UT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a dynamic origin, unit spacing, and dynamic transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest6( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D DO = createOrigin2DFromStageTiles<Origin2D,OriginRow,OriginPoint,
      itk::TileLayout2D,itk::TileRow2D,itk::Tile2D>(stageTiles);

  Spacing2D US = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 1);

  Transform2D DT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, DO, US, DT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * Helper Method that creates a dynamic origin, dynamic spacing, and dynamic transform and passes them through
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType >
int
executeInMemoryMontageTest7( const itk::TileLayout2D& stageTiles, const std::string& inputPath,
                            const std::string& outFilename, unsigned streamSubdivisions)
{
  constexpr unsigned Dimension = 2;
  using ScalarPixelType = typename itk::NumericTraits< PixelType >::ValueType;
  using ScalarImageType = itk::Image< ScalarPixelType, Dimension >;
  using SpacingType = typename ScalarImageType::SpacingType;
  using SpacingRow = std::vector<SpacingType>;
  using Spacing2D = std::vector<SpacingRow>;
  using MontageType = itk::TileMontage< ScalarImageType >;
  using TransformPtr = typename MontageType::TransformPointer;
  using TransformPtrRow = std::vector<TransformPtr>;
  using Transform2D = std::vector<TransformPtrRow>;
  using OriginPoint = typename ScalarImageType::PointType;
  using OriginRow = std::vector<OriginPoint>;
  using Origin2D = std::vector<OriginRow>;

  unsigned yMontageSize = stageTiles.size();
  unsigned xMontageSize = stageTiles[0].size();

  Origin2D DO = createOrigin2DFromStageTiles<Origin2D,OriginRow,OriginPoint,
      itk::TileLayout2D,itk::TileRow2D,itk::Tile2D>(stageTiles);

  Spacing2D DS = createSpacing2D<Spacing2D,SpacingRow,SpacingType,Dimension>(yMontageSize, xMontageSize, 0.5);

  Transform2D DT = createUnitTransform2D<Transform2D,TransformPtrRow,MontageType>(yMontageSize, xMontageSize);

  return executeInMemoryMontageTest<PixelType,AccumulatePixelType,Origin2D,Spacing2D,
      Transform2D>(stageTiles, inputPath, outFilename, DO, DS, DT, streamSubdivisions);
}

/* -----------------------------------------------------------------------------------------------
 * This method tests stitching images with different combinations of origins, spacings, and
 * transforms.
 *
 * Legend:
 * UO - Unit Origin.  An origin of (0, 0, 0).
 * US - Unit Spacing.  A spacing of (1, 1, 1).
 *                                                (1, 0, 0)
 * UT - Unit Transform.  An identity transform of (0, 1, 0)
 *                                                (0, 0, 1)
 *
 * DO - Dynamic Origin.  An origin that does not equal the unit origin, such as (12, 0, 51)
 * DS - Dynamic Spacing.  Spacing that is not identical to unit spacing, such as (0.5, 0.25, 0.5)
 * DT - Dynamic Transform.  A transform that is not identical to a unit transform.
 *
 * ~~~~~~~~~~~
 *
 * Each run of the test uses either a unit or dynamic origin, spacing, and transform.  In total,
 * there are 8 combinations...
 *
 * Combinations:
 * UO | US | UT - Fail
 * UO | DS | UT - Fail
 * UO | US | DT
 * UO | DS | DT
 *
 * DO | US | UT
 * DO | DS | UT
 * DO | US | DT
 * DO | DS | DT
 * ----------------------------------------------------------------------------------------------- */
template< typename PixelType, typename AccumulatePixelType, typename TestVariation >
int
inMemoryMontageTest( const itk::TileLayout2D& stageTiles, const std::string& inputPath, std::string& outFilename,
                     TestVariation variation, unsigned streamSubdivisions)
{
  outFilename = outFilename + std::to_string( static_cast<unsigned int>(variation) ) + ".mha";

  if (variation == TestVariation::UOrigin_USpacing_UTransform)
  {
    return executeInMemoryMontageTest0<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::UOrigin_DSpacing_UTransform)
  {
    return executeInMemoryMontageTest1<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::UOrigin_USpacing_DTransform)
  {
    return executeInMemoryMontageTest2<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::UOrigin_DSpacing_DTransform)
  {
    return executeInMemoryMontageTest3<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::DOrigin_USpacing_UTransform)
  {
    return executeInMemoryMontageTest4<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::DOrigin_DSpacing_UTransform)
  {
    return executeInMemoryMontageTest5<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::DOrigin_USpacing_DTransform)
  {
    return executeInMemoryMontageTest6<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else if (variation == TestVariation::DOrigin_DSpacing_DTransform)
  {
    return executeInMemoryMontageTest7<PixelType,AccumulatePixelType>(stageTiles, inputPath,
                                                                      outFilename, streamSubdivisions);
  }
  else
  {
    return EXIT_FAILURE;
  }
}

#endif // itkMontageTestHelper_hxx
