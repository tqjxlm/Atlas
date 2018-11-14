/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2016 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <osgEarth/TileSource>
#include <osgEarth/FileUtils>
#include <osgEarth/ImageUtils>
#include <osgEarth/Registry>
#include <osgEarth/ImageToHeightFieldConverter>

#include <osg/Notify>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReadFile>

#include <OpenThreads/Atomic>

#include <sstream>
#include <iomanip>
#include <string.h>

#include "HeightMapOptions"

using namespace osgEarth;
using namespace osgEarth::Drivers;

#define LC "[HeightMap Driver] "
#define OE_TEST OE_DEBUG


class HeightMapSource : public TileSource
{
public:
	HeightMapSource(const TileSourceOptions& options) :
        TileSource(options), _options(options), _rotate_iter(0u), _rotateStart(0), _rotateEnd(0)
    {
        //nop
    }


    Status initialize(const osgDB::Options* dbOptions)
    {
        _dbOptions = Registry::instance()->cloneOrCreateOptions(dbOptions);        

        URI xyzURI = _options.url().value();
        if ( xyzURI.empty() )
        {
            return Status::Error( Status::ConfigurationError, "Fail: driver requires a valid \"url\" property" );
        }
		
        // driver requires a profile.
        if ( !getProfile() )
        {
            return Status::Error( Status::ConfigurationError, "An explicit profile definition is required by the XYZ driver." );
        }

        _template = xyzURI.full();

        _format = _options.format().isSet() 
            ? *_options.format()
            : osgDB::getLowerCaseFileExtension( xyzURI.base() );

        return STATUS_OK;
    }


    osg::Image* createImage(const TileKey&     key,
                            ProgressCallback*  progress )
    {
        unsigned x, y;
        key.getTileXY( x, y );

        if ( _options.invertY() == true )
        {
            unsigned cols=0, rows=0;
            key.getProfile()->getNumTiles( key.getLevelOfDetail(), cols, rows );
            y = rows - y - 1;
        }

        std::string location = _template;
		location += '/' + std::to_string(key.getLevelOfDetail()) + '/' + std::to_string(x) + '/' + std::to_string(y) + '.' + _format;

        std::string cacheKey;

        URI uri( location, _options.url()->context() );
        if ( !cacheKey.empty() )
            uri.setCacheKey( cacheKey );

        OE_TEST << LC << "URI: " << uri.full() << ", key: " << uri.cacheKey() << std::endl;

        return uri.getImage( _dbOptions.get(), progress );
    }

    virtual std::string getExtension() const 
    {
        return _format;
    }

	osg::HeightField*
		createHeightField(const TileKey&        key,
			ProgressCallback*     progress)
	{

			osg::ref_ptr<osg::Image> image = createImage(key, progress);
			osg::HeightField* hf = 0;
			if (image.valid())
			{
				ImageToHeightFieldConverter conv;
				hf = conv.convert(image.get());
			}
			return hf;
	}

private:
    const HeightMapOptions       _options;
    std::string            _format;
    std::string            _template;
    std::string            _rotateChoices;
    std::string            _rotateString;
    std::string::size_type _rotateStart, _rotateEnd;
    OpenThreads::Atomic    _rotate_iter;

    osg::ref_ptr<osgDB::Options> _dbOptions;
};




class HeightMapTileSourceDriver : public TileSourceDriver
{
public:
	HeightMapTileSourceDriver()
    {
        supportsExtension( "osgearth_HeightMap", "HeightMap Driver" );
    }

    virtual const char* className() const
    {
        return "HeightMap Driver";
    }

    virtual ReadResult readObject(const std::string& file_name, const Options* options) const
    {
        if ( !acceptsExtension(osgDB::getLowerCaseFileExtension( file_name )))
            return ReadResult::FILE_NOT_HANDLED;

        return new HeightMapSource( getTileSourceOptions(options) );
    }
};

REGISTER_OSGPLUGIN(osgearth_HeightMap, HeightMapTileSourceDriver)
