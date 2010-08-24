/*
 * BezierDataSource.h
 *
 * Copyright (C) 2009 by VISUS (Universitaet Stuttgart)
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOLCORE_BEZIERDATASOURCE_H_INCLUDED
#define MEGAMOLCORE_BEZIERDATASOURCE_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#include "BezierDataCall.h"
#include "CalleeSlot.h"
#include "Module.h"
#include "param/ParamSlot.h"
#include "vislib/Array.h"
#include "vislib/BezierCurve.h"


namespace megamol {
namespace core {
namespace misc {


    /**
     * Data loader module for 3+1 dim cubic b�zier data
     */
    class BezierDataSource : public Module {
    public:

        /**
         * Answer the name of this module.
         *
         * @return The name of this module.
         */
        static const char *ClassName(void) {
            return "BezierDataSource";
        }

        /**
         * Answer a human readable description of this module.
         *
         * @return A human readable description of this module.
         */
        static const char *Description(void) {
            return "Data source module for Bezier data.";
        }

        /**
         * Answers whether this module is available on the current system.
         *
         * @return 'true' if the module is available, 'false' otherwise.
         */
        static bool IsAvailable(void) {
            return true;
        }

        /** Ctor. */
        BezierDataSource(void);

        /** Dtor. */
        virtual ~BezierDataSource(void);

    protected:

        /**
         * Implementation of 'Create'.
         *
         * @return 'true' on success, 'false' otherwise.
         */
        virtual bool create(void);

        /**
         * Implementation of 'Release'.
         */
        virtual void release(void);

    private:

        /**
         * Gets the data from the source.
         *
         * @param caller The calling call.
         *
         * @return 'true' on success, 'false' on failure.
         */
        bool getDataCallback(Call& caller);

        /**
         * Gets the data from the source.
         *
         * @param caller The calling call.
         *
         * @return 'true' on success, 'false' on failure.
         */
        bool getExtentCallback(Call& caller);

        /**
         * Ensures that the data file is loaded into memory, if possible
         */
        void assertData(void);

        /**
         * Load a BezDat file into the memory
         *
         * @param filename The path of the file to load
         */
        void loadBezDat(const vislib::TString& filename);

        /** The file name */
        param::ParamSlot filenameSlot;

        /** The slot for requesting data */
        CalleeSlot getDataSlot;

        /** The bounding box of positions*/
        float minX, minY, minZ, maxX, maxY, maxZ;

        /** The curves data */
        vislib::Array<vislib::math::BezierCurve<
            BezierDataCall::BezierPoint, 3> > curves;

        /** The hash value of the loaded data */
        SIZE_T datahash;

    };

} /* end namespace misc */
} /* end namespace core */
} /* end namespace megamol */

#endif /* MEGAMOLCORE_BEZIERDATASOURCE_H_INCLUDED */
