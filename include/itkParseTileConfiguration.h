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

#include <string>
#include <fstream>
#include <sstream>

std::string
getNextNonCommentLine( std::istream& in )
{
  std::string temp;
  while ( std::getline( in, temp ) )
    {
    // this is neither an empty line nor a comment
    if ( !temp.empty() && temp[0] != '#' )
      {
      break;
      }
    }
  return temp;
}

template< unsigned Dimension >
struct Tile
{
  itk::Point< double, Dimension > Position; // x, y... coordinates

  std::string FileName;
};

template< unsigned Dimension >
Tile< Dimension >
parseLine( const std::string line, std::string& timePointID )
{
  Tile< Dimension > tile;
  std::stringstream ss( line );
  std::string temp;

  std::getline( ss, temp, ';' );
  tile.FileName = temp;
  std::getline( ss, temp, ';' );
  timePointID = temp;
  std::getline( ss, temp, '(' );

  using PointType = itk::Point< double, Dimension >;
  PointType p;
  for (unsigned d = 0; d < Dimension; d++)
    {
    ss >> p[d]; //TODO: use DoubleConversion here!
    ss.ignore();
    }
  tile.Position = p;

  return tile;
}

template< unsigned Dimension >
std::vector< Tile< Dimension > >
parseRow( std::string& line, std::istream& in, std::string& timePointID )
{
  std::vector< Tile< Dimension > > row;

  Tile< Dimension > tile = parseLine< Dimension >( line, timePointID );
  row.push_back( tile );
  line = getNextNonCommentLine( in );

  while (in)
    {
    tile = parseLine< Dimension >( line, timePointID );
    if ( tile.Position[0] < row.back().Position[0] ) // this is start of a new row
      {
      return row;
      }
    row.push_back( tile );
    line = getNextNonCommentLine( in );
    }

  return row;
}


std::vector< std::vector< Tile< 2 > > >
parseTileConfiguration2D( const std::string pathToFile )
{
  constexpr unsigned Dimension = 2;
  using PointType = itk::Point< double, Dimension >;
  using TileRow = std::vector< Tile< Dimension > >;
  using TileLayout = std::vector< TileRow >;

  unsigned xMontageSize = 0;
  TileLayout tiles;
  std::string timePointID; // just to make sure all lines specify the same time point

  std::ifstream tileFile( pathToFile );
  std::string temp = getNextNonCommentLine( tileFile );
  if (temp.substr(0, 6) == "dim = ")
    {
    unsigned dim = std::stoul( temp.substr( 6 ) );
    if (dim != Dimension)
      {
      throw std::runtime_error( "Expected dimension 2, but got " + std::to_string( dim ) + " from string:\n\n" + temp );
      }
    temp = getNextNonCommentLine( tileFile ); //get next line
    }

  // read coordinates from files
  while ( tileFile )
    {
    TileRow tileRow = parseRow< Dimension >( temp, tileFile, timePointID );
    if (xMontageSize == 0)
      {
      xMontageSize = tileRow.size(); // we get size from first row
      }
    else // check it is the same size as first row
      {
      assert( xMontageSize == tileRow.size() );
      }
    tiles.push_back( tileRow );
    }

  return tiles;
}

#endif // itkParseTileConfiguration_h
