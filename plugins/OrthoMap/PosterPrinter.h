#ifndef OSGPOSTER_POSTERPRINTER
#define OSGPOSTER_POSTERPRINTER

#include <osg/NodeVisitor>
#include <osgViewer/Renderer>

#include <string>
#include <set>

#include <QMessageBox>
#include <QProgressDialog>
#include <QTime>

#include <ViewerWidget/ViewerWidget.h>

/** PosterVisitor: A visitor for adding culling callbacks to newly allocated paged nodes */
class PosterVisitor : public osg::NodeVisitor
{
public:
    typedef std::set<std::string> PagedNodeNameSet;
    
    PosterVisitor();
    META_NodeVisitor( osgPoster, PosterVisitor );
    
    void insertName( const std::string& name )
	{
		if (_pagedNodeNames.insert(name).second)
		{
			_needToApplyCount++;
			_namesToApply.insert(name);
		}
	}
    
    void eraseName( const std::string& name )
	{
		if (_pagedNodeNames.erase(name) > 0)
		{
			_needToApplyCount--;
			//_namesToApply.erase(name);
		}
	}

	void clearNames() { _pagedNodeNames.clear(); _namesToApply.clear();  _needToApplyCount = 0; _appliedCount = 0; }
    unsigned int getNumNames() const { return _pagedNodeNames.size(); }
    
    PagedNodeNameSet& getPagedNodeNames() { return _pagedNodeNames; }
    const PagedNodeNameSet& getPagedNodeNames() const { return _pagedNodeNames; }

	PagedNodeNameSet& getNamesToApply() { return _namesToApply; }
	const PagedNodeNameSet& getNamesToApply() const { return _namesToApply; }
    
    unsigned int getNeedToApplyCount() const { return _needToApplyCount; }
    unsigned int getAppliedCount() const { return _appliedCount; }
    unsigned int inQueue() const { return _needToApplyCount>_appliedCount ? _needToApplyCount-_appliedCount : 0; }
    
    void setAddingCallbacks( bool b ) { _addingCallbacks = b; }
    bool getAddingCallbacks() const { return _addingCallbacks; }

	void setResolvingMissedTiles(bool b) { _resolvingMissedTiles = b; }
    
    virtual void apply( osg::LOD& node );
    virtual void apply( osg::PagedLOD& node );
    
protected:
    bool hasCullCallback( osg::Callback* nc, osg::Callback* target )
    {
        if ( nc==target ) return true;
        else if ( !nc ) return false;
        return hasCullCallback( nc->getNestedCallback(), target );
    }
    
    PagedNodeNameSet _pagedNodeNames;
	PagedNodeNameSet _namesToApply;
    unsigned int _appliedCount;
    unsigned int _needToApplyCount;
    bool _addingCallbacks;
	bool _resolvingMissedTiles;
};

/** PosterIntersector: A simple polytope intersector for updating pagedLODs in each image-tile */
class PosterIntersector : public osgUtil::Intersector
{
public:
    typedef std::set<std::string> PagedNodeNameSet;
    
    PosterIntersector( const osg::Polytope& polytope );
    PosterIntersector( double xMin, double yMin, double xMax, double yMax );
    
    void setPosterVisitor( PosterVisitor* pcv ) { _visitor = pcv; }
    PosterVisitor* getPosterVisitor() { return _visitor.get(); }
    const PosterVisitor* getPosterVisitor() const { return _visitor.get(); }
    
    virtual Intersector* clone( osgUtil::IntersectionVisitor& iv );
    
    virtual bool containsIntersections()
    { return _visitor.valid()&&_visitor->getNumNames()>0; }
    
    virtual bool enter( const osg::Node& node );
    virtual void leave() {}
    virtual void reset();
    virtual void intersect( osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable );
    
protected:
    osgUtil::IntersectionVisitor* _intersectionVisitor;
    osg::ref_ptr<PosterVisitor> _visitor;
    PosterIntersector* _parent;
    osg::Polytope _polytope;
	osg::Polytope _userArea;
};

/** PosterPrinter: The implementation class of high-res rendering */
class PosterPrinter : public osg::Referenced
{
public:
    typedef std::pair<unsigned int, unsigned int> TilePosition;
    typedef std::map< TilePosition, osg::ref_ptr<osg::Image> > TileImages;

	enum OutputType {
		RGB,
		DEPTH
	};
    
    PosterPrinter();
    
    /** Set to output each sub-image-tile to disk */
    void setOutputTiles( bool b ) { _outputTiles = b; }
    bool getOutputTiles() const { return _outputTiles; }
    
    /** Set the output sub-image-tile extension, e.g. bmp */
    void setOutputTileExtension( const std::string& ext ) { _outputTileExt = ext; }
    const std::string& getOutputTileExtension() const { return _outputTileExt; }
    
    /** Set the output poster name, e.g. output.bmp */
    void setOutputPosterName( const std::string& name ) { _outputPosterName = name; }
    const std::string& getOutputPosterName() const { return _outputPosterName; }
    
    /** Set the size of each sub-image-tile, e.g. 640x480 */
    void setTileSize( int w, int h ) { _tileSize.set(w, h); }
    const osg::Vec2i& getTileSize() const { return _tileSize; }
    
    /** Set the final size of the high-res poster, e.g. 6400x4800 */
    void setPosterSize( int w, int h ) { _posterSize.set(w, h); }
    const osg::Vec2i& getPosterSize() const { return _posterSize; }
    
    /** Set the capturing camera */
    void setCamera( osg::Camera* camera ) { _camera = camera; }
    const osg::Camera* getCamera() const { return _camera.get(); }
    
    /** Set the final poster image, should be already allocated */
    void setFinalPoster( osg::Image* image ) { _finalPoster = image; }
    const osg::Image* getFinalPoster() const { return _finalPoster.get(); }
    
    PosterVisitor* getPosterVisitor() { return _visitor.get(); }
    const PosterVisitor* getPosterVisitor() const { return _visitor.get(); }

	void setPixel(double pixelPerMeter) { _pixelPerMeter = pixelPerMeter; }
	void setPath(const QString& path) { _qPath = path; }
	void setViewCamera( osg::Camera* camera) { _viewCamera = camera; }
	void setOutputType(OutputType outputType) { 
		_outputType = outputType; 
		switch (_outputType)
		{
		case(RGB):
			_bufferType = osg::Camera::COLOR_BUFFER;
			_pixelFormat = GL_RGB;
			_dataFormat = GL_UNSIGNED_BYTE;
			_pixelSize = 3;
			_dataSize = sizeof(unsigned char);
			break;
		case(DEPTH):
			_bufferType = osg::Camera::DEPTH_BUFFER;
			_pixelFormat = GL_DEPTH_COMPONENT;
			_dataFormat = GL_FLOAT;
			_pixelSize = 1;
			_dataSize = sizeof(float);
			break;
		default:
			_bufferType = osg::Camera::COLOR_BUFFER;
			_pixelFormat = GL_RGB;
			_dataFormat = GL_UNSIGNED_BYTE;
			_pixelSize = 3;
			_dataSize = sizeof(unsigned char);
			break;
		}
	}
    
    bool done() const { return !_isRunning && !_isFinishing; }
    
    void init( const osg::Camera* camera );
    void init( const osg::Matrixd& view, const osg::Matrixd& proj );
    void frame( const osg::FrameStamp* fs, osg::Node* node );
	QString& getInfo() {return _info;}
    
protected:
    virtual ~PosterPrinter() {_images.clear();}
    
    bool addCullCallbacks( const osg::FrameStamp* fs, osg::Node* node );
    void removeCullCallbacks( osg::Node* node );
    void bindCameraToImage( osg::Camera* camera, int row, int col );
    void recordImages();
	void calcuZvaluefromDepth(osg::Image * image, int col, int row);

	OutputType _outputType;
	osg::Camera::BufferComponent _bufferType;
	GLenum _pixelFormat;
	GLenum _dataFormat;
	unsigned short _pixelSize;
	unsigned short _dataSize;

	QString _info;
    
    bool _outputTiles;
    std::string _outputTileExt;
    std::string _outputPosterName;
    osg::Vec2i _tileSize;
    osg::Vec2i _posterSize;
    
    bool _isRunning;
    bool _isFinishing;
    unsigned int _lastBindingFrame;
    int _tileRows;
    int _tileColumns;
    int _currentRow;
    int _currentColumn;
    osg::ref_ptr<PosterIntersector> _intersector;
    osg::ref_ptr<PosterVisitor> _visitor;
    
    osg::Matrixd _currentViewMatrix;
    osg::Matrixd _currentProjectionMatrix;
    osg::ref_ptr<osg::Camera> _camera;
	osg::ref_ptr<osg::Camera> _viewCamera;
    osg::ref_ptr<osg::Image> _finalPoster;
    TileImages _images;

	double _pixelPerMeter;
	QString _qPath;
};

class CustomRenderer : public osgViewer::Renderer
{
public:
	CustomRenderer(osg::Camera* camera)
		: osgViewer::Renderer(camera), _cullOnly(true)
	{
	}

	void setCullOnly(bool on) { _cullOnly = on; }

	virtual void operator ()(osg::GraphicsContext*)
	{
		if (_graphicsThreadDoesCull)
		{
			if (_cullOnly) cull();
			else cull_draw();
		}
	}

	virtual void cull()
	{
		osgUtil::SceneView* sceneView = _sceneView[0].get();
		if (!sceneView || _done || _graphicsThreadDoesCull)
			return;

		updateSceneView(sceneView);

		osgViewer::View* view = dynamic_cast<osgViewer::View*>(_camera->getView());
		if (view)
			sceneView->setFusionDistance(view->getFusionDistanceMode(), view->getFusionDistanceValue());
		sceneView->inheritCullSettings(*(sceneView->getCamera()));
		sceneView->cull();
	}

	bool _cullOnly;
};

class PrintPosterHandler : public QObject, public osgGA::GUIEventHandler
{
	Q_OBJECT

public:
	PrintPosterHandler(PosterPrinter* printer)
		: _printer(printer), _started(false) {}

	bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
	{
		osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
		if (!view) return false;

		switch (ea.getEventType())
		{
		case osgGA::GUIEventAdapter::FRAME:
			if (view->getDatabasePager())
			{
				// Wait until all paged nodes are processed
				if (view->getDatabasePager()->getRequestsInProgress())
					break;
			}

			if (_printer.valid())
			{
				_printer->frame(view->getFrameStamp(), view->getSceneData());
				emit nextStage(_printer->getInfo());
				if (_started && _printer->done())
				{
					osg::Switch* root = dynamic_cast<osg::Switch*>(view->getSceneData());
					if (root)
					{
						// Assume child 0 is the loaded model and 1 is the poster camera
						// Switch them in time to prevent dual traversals of subgraph
						root->setValue(0, true);
						root->setValue(1, false);
					}
					_started = false;
					emit isWorking(false);
				}
			}
			break;

		case osgGA::GUIEventAdapter::KEYDOWN:
			if (ea.getKey() == 'p' || ea.getKey() == 'P')
			{
				if (_printer.valid())
				{
					osg::Switch* root = dynamic_cast<osg::Switch*>(view->getSceneData());
					//if ( root )
					//{
					//	// Assume child 0 is the loaded model and 1 is the poster camera
					//	root->setValue( 0, false );
					//	root->setValue( 1, true );
					//}

					_printer->init(view->getCamera());
					_started = true;
					emit isWorking(true);
				}
				return true;
			}
			break;

		default:
			break;
		}
		return false;
	}

signals:
	void nextStage(const QString&);
	void isWorking(bool);

protected:
	osg::ref_ptr<PosterPrinter> _printer;
	bool _started;
};

class WaitProgressDialog : public QProgressDialog
{
	Q_OBJECT

public:
	WaitProgressDialog(const QString& labelText, const QString& cancelButtonText,
		int minimum, int maximum, QWidget *parent = 0, Qt::WindowFlags flags = 0)
		: QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, flags)
	{
		setCancelButton(0);
		_processTime.setHMS(0, 0, 0, 0);
		setAttribute(Qt::WA_DeleteOnClose, true);
	}

	~WaitProgressDialog() {}

	public slots:
	void updateTime()
	{
		_processTime = _processTime.addSecs(1);
		setLabelText(_msg + "\n" + _processTime.toString());
	}

	void updateMessage(const QString& msg)
	{
		_msg = msg;
		setLabelText(_msg + "\n" + _processTime.toString());
	}

private:
	QTime _processTime;
	QString _msg;
};

#endif //OSGPOSTER_POSTERPRINTER