typedef uint16_t tove_gpu_float_t;
// rgba32f support:
// typedef float tove_gpu_float_t;

typedef enum {
	TOVE_REPORT_DEBUG = 1,
	TOVE_REPORT_SLOW,
	TOVE_REPORT_WARN,
	TOVE_REPORT_ERR,
	TOVE_REPORT_FATAL
} ToveReportLevel;

typedef void (*ToveReportFunction)(
	const char *s, ToveReportLevel level);

typedef uint32_t ToveChangeFlags;

typedef uint32_t ToveMeshUpdateFlags;

typedef struct {
	ToveMeshUpdateFlags update;
} ToveMeshResult;

typedef enum {
	TRIANGLES_LIST,
	TRIANGLES_STRIP
} ToveTrianglesMode;

typedef enum {
	TOVE_GLSL2,
	TOVE_GLSL3
} ToveShaderLanguage;

typedef enum {
	TOVE_POINT,
	TOVE_CURVE,
	TOVE_SUBPATH,
	TOVE_PATH
} ToveElementType;

typedef uint16_t ToveVertexIndex;

typedef struct {
	ToveTrianglesMode mode;
	const ToveVertexIndex *array;
	int size;
} ToveTriangles;

typedef enum {
	PAINT_UNDEFINED = -1,
	PAINT_NONE = 0,
	PAINT_SOLID = 1,
	PAINT_SHADER = 2,
	PAINT_LINEAR_GRADIENT = 3,
	PAINT_RADIAL_GRADIENT = 4
} TovePaintType;

typedef enum {
	TOVE_FILLRULE_NON_ZERO = 0,
	TOVE_FILLRULE_EVEN_ODD = 1
} ToveFillRule;

typedef enum {
	TOVE_LINEJOIN_MITER,
	TOVE_LINEJOIN_ROUND,
	TOVE_LINEJOIN_BEVEL
} ToveLineJoin;

typedef enum {
	TOVE_LINECAP_BUTT,
	TOVE_LINECAP_ROUND,
	TOVE_LINECAP_SQUARE
} ToveLineCap;

typedef enum {
	TOVE_ORIENTATION_CW = 0,
	TOVE_ORIENTATION_CCW = 1
} ToveOrientation;

typedef enum {
	TOVE_HANDLE_FREE,
	TOVE_HANDLE_ALIGNED
} ToveHandle;

enum {
	CHANGED_FILL_STYLE = 1,
	CHANGED_LINE_STYLE = 2,
	CHANGED_POINTS = 4,
	CHANGED_GEOMETRY = 8,
	CHANGED_LINE_ARGS = 16,
	CHANGED_BOUNDS = 32,
	CHANGED_EXACT_BOUNDS = 64,
	CHANGED_RECREATE = 128,
	CHANGED_FILL_ARGS = 256,
	CHANGED_INITIAL = 1024,
	CHANGED_PAINT_INDICES = 2048,
	CHANGED_ANYTHING = 511,
	CHANGED_COLORS = CHANGED_FILL_STYLE | CHANGED_LINE_STYLE
};

enum {
	UPDATE_MESH_VERTICES = 1,
	UPDATE_MESH_COLORS = 2,
	UPDATE_MESH_TRIANGLES = 4,
	UPDATE_MESH_GEOMETRY = 8,
	UPDATE_MESH_EVERYTHING = 15,
	UPDATE_MESH_AUTO_TRIANGLES = 128
};

typedef struct {
	float x, y;
} TovePoint;

typedef struct {
	void *ptr;
} TovePaintRef;

typedef struct {
	void *ptr;
} ToveSubpathRef;

typedef struct {
	void *ptr;
} ToveCommandRef;

typedef struct {
	void *ptr;
} TovePathRef;

typedef struct {
	void *ptr;
} ToveGraphicsRef;

typedef struct {
	void *ptr;
} ToveTesselatorRef;

typedef struct {
	void *ptr;
} TovePaletteRef;

typedef struct {
	void *ptr;
} ToveNameRef;

typedef enum {
	TOVE_REC_DEPTH,
	TOVE_ANTIGRAIN,
	TOVE_MAX_ERROR
} ToveStopCriterion;

typedef struct {
	float distanceTolerance;
	float colinearityEpsilon;
	float angleEpsilon;
	float angleTolerance;
	float cuspLimit;
} AntiGrainSettings;

typedef struct {
	ToveStopCriterion stopCriterion;
	int recursionLimit;
	float arcTolerance;
	AntiGrainSettings antigrain;
	float maximumError;
	float clipperScale;
} ToveTesselationSettings;

typedef enum {
	TOVE_DITHER_NONE = 0,
	TOVE_DITHER_DIFFUSION = 1,
	TOVE_DITHER_ORDERED = 2
} ToveDitherType;

typedef struct {
	ToveDitherType type;
	const float *matrix;
	uint16_t matrix_width;
	uint16_t matrix_height;
	float spread;
} ToveDither;

typedef struct {
	float tessTolerance;
	float distTolerance;
	struct {
		ToveDither dither;
		struct {
			float amount;
			const float *matrix;
			int16_t n;
		} noise;
		TovePaletteRef palette;
	} quality;
} ToveRasterizeSettings;

typedef struct {
	float r, g, b, a;
} ToveRGBA;

typedef struct {
	ToveRGBA rgba;
	float offset;
} ToveColorStop;

typedef struct {
	float values[6];
} ToveGradientParameters;

typedef struct {
	union {
		float bounds[4];
		struct {
			float x0;
			float y0;
			float x1;
			float y1;
		};
	};
} ToveBounds;

typedef struct {
	float x, y;
} ToveVec2;

typedef struct {
	float x, y, z, w;
} ToveVec4;

typedef struct {
	float t;
	float distanceSquared;
} ToveNearest;

typedef struct {
	int16_t numPaints;
	int16_t numGradients;
	int16_t numColors;
} TovePaintColorAllocation;

typedef struct {
	int16_t numGradients;
	int16_t numColors;
	int8_t matrixRows;
	float *matrix;
	float *arguments;
	uint8_t *colorsTexture;
	int16_t colorsTextureRowBytes;
	int16_t colorsTextureHeight;
} ToveGradientData;

typedef struct {
	int8_t style; // TovePaintType
	ToveRGBA rgba;
	ToveGradientData gradient;
	void *shader;
} TovePaintData;

typedef struct {
	TovePaintData *array;
	int16_t size;
} TovePaintDataArray;

typedef struct {
	int n[2]; // number of used lookup table elements for x and y
	int bsearch; // number of needed (compound) bsearch iterations
} ToveLookupTableMeta;

typedef struct {
	int16_t curveIndex;
	int16_t numCurves;
	bool isClosed;
} ToveLineRun;

typedef struct {
	int16_t maxCurves;
	int16_t numCurves;
	int16_t numSubPaths;

	ToveBounds *bounds;
	float strokeWidth;
	float miterLimit;
	int8_t fillRule;
	bool fragmentShaderLine;
	bool opaqueLine;
	ToveLineRun *lineRuns;

	float *lookupTable[2];
	int16_t lookupTableSize;
	ToveLookupTableMeta *lookupTableMeta;

// #if TOVE_GPUX_MESH_BAND
	float *bandsVertices[3];
	int16_t numBandsVertices[3];
	int16_t maxBandsVertices;
// #endif

	uint8_t *listsTexture;
	int listsTextureRowBytes;
	int listsTextureSize[2];
	const char *listsTextureFormat;

	tove_gpu_float_t *curvesTexture;
	int curvesTextureRowBytes;
	int curvesTextureSize[2];
	const char *curvesTextureFormat;
} ToveShaderGeometryData;

typedef struct {
	TovePaintData line;
	TovePaintData fill;
} ToveShaderLineFillColorData;

typedef struct {
	ToveShaderLineFillColorData color;
	ToveShaderGeometryData geometry;
	bool hasNormalFillStrokeOrder;
} ToveShaderData;

typedef struct {
	void *ptr;
} ToveFeedRef;

typedef struct {
	void *ptr;
} ToveMeshRef;

typedef struct {
	float curvature;
	float balance;
	float angle1;
	float angle2;
} ToveCurvature;

typedef struct {
	const char *code;
	uint32_t embedded[2];
} ToveShaderCode;

typedef struct {
	// the following values are uint32_t by design: these will be unboxed,
	// will fit into Lua doubles and can act as hash table values in Lua.
	// uint64_t would not work.

	// http://luajit.org/ext_ffi_semantics.html
	//	 - Conversions from C types to Lua objects
	//   - cdata objects as table keys
	// https://stackoverflow.com/questions/43655668/
	//   are-all-integer-values-perfectly-represented-as-doubles

	// at 100 fps, version will work for > 1 year without overflowing.

	uint32_t id; // >= 1
	uint32_t version;
} ToveSendArgs;
