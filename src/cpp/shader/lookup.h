/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#ifndef __TOVE_SHADER_LOOKUP
#define __TOVE_SHADER_LOOKUP 1

#include <unordered_set>
#include <vector>

#include "curvedata.h"

enum EventType {
    EVENT_ENTER,
    EVENT_EXIT,
	EVENT_MARK
};

struct Event {
    float y;
    EventType t : 8;
    uint8_t curve;
};

enum {
    SENTINEL_END = 0xff,
    SENTINEL_STROKES = 0xfe
};

class LookupTable {
public:
	typedef std::unordered_set<uint8_t> CurveSet;

private:
	ToveShaderGeometryData &_data;
    CurveSet active;
    const int _numCurves;
	const int _ignore;

public:
    LookupTable(int numCurves, ToveShaderGeometryData &data, int ignore) :
		_numCurves(numCurves), _data(data), _ignore(ignore) {

        active.reserve(numCurves);
    }

	void dump(ToveShaderGeometryData *data) {
#if 0
		printf("-- dump lut --\n");
		printf("numCurves: %d\n", _numCurves);
		for (int i = 0; i < 2 * (_numCurves + 2); i++) {
			printf("lut %03d: %f (%s)\n", i, data->lookupTable[i], (i & 1) == 0 ? "x" : "y");
		}

		for (int y = 0; y < data->listsTextureSize[1]; y++) {
			if (y == data->listsTextureSize[1] / 2) {
				printf("---------\n");
			}
			printf("lists %03d: ", y);
			uint8_t *yptr = data->listsTexture + y * data->listsTextureRowBytes;
			for (int x = 0; x < data->listsTextureSize[0]; x++) {
				printf("%d ", yptr[x]);
			}
			printf("\n");
		}
		printf("-- end dump lut --\n");
#endif
	}

    template<typename F>
    void build(int dim, const std::vector<Event> &events,
		int numEvents, const std::vector<ExtendedCurveData> &extended,
		float padding, const F &finish) {

		ToveShaderGeometryData &data = _data;
		const int ignore = _ignore;

        active.clear();
        float *lookupTable = data.lookupTable + dim;

        auto i = events.cbegin();
        const auto end = events.cbegin() + numEvents;
        float *ylookup = lookupTable;

		uint8_t *yptr = data.listsTexture;
		int rowBytes = data.listsTextureRowBytes;

		yptr += dim * rowBytes * (data.listsTextureSize[1] / 2);

        float y0;

        while (i != end) {
            const float y = i->y;

            if (ylookup == lookupTable) {
                if (padding > 0.0) {
                    y0 = y - padding;
                    *ylookup = y0;
					ylookup += 2;
                    finish(y0, y, active, yptr);
					yptr += rowBytes;
                } else {
                    y0 = y;
                }
            }

            auto j = i;
            while (j != end && j->y == y) {
                switch (j->t) {
					case EVENT_ENTER:
                    	active.insert(j->curve);
						break;
					case EVENT_EXIT:
                    	active.erase(j->curve);
						break;
					case EVENT_MARK:
						break;
                }
                j++;
            }

            *ylookup = y;
			ylookup += 2;

            int k = 0;
            assert(active.size() <= _numCurves);
            for (auto a = active.cbegin(); a != active.cend(); a++) {
				const int curve = *a;
				if ((extended[curve].ignore & ignore) == 0) {
					yptr[k++] = curve;
				}
            }

            finish(y0, y, active, &yptr[k]);

            i = j;
            y0 = y;
			yptr += rowBytes;
        }

        if (ylookup > lookupTable && padding > 0.0) {
            float y0 = ylookup[-2];
            float y1 = y0 + padding;
            *ylookup = y1;
			ylookup += 2;
            finish(y0, y1, active, yptr);
			yptr += rowBytes;
        }

        data.lookupTableFill[dim] = (ylookup - lookupTable) / 2;
		assert(data.lookupTableFill[dim] <= data.lookupTableSize);
    }

    inline void build(int dim, const std::vector<Event> &events,
		int numEvents, const std::vector<ExtendedCurveData> &extended) {
        build(dim, events, numEvents, extended, 0.0,
			[] (float y0, float y1, const CurveSet &active, uint8_t *list) {
            	*list = SENTINEL_END;
        	});
    }
};

#endif // __TOVE_SHADER_LOOKUP
