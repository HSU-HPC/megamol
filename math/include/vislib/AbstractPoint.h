/*
 * AbstractPoint.h
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 */

#ifndef VISLIB_ABSTRACTPOINT_H_INCLUDED
#define VISLIB_ABSTRACTPOINT_H_INCLUDED
#if (_MSC_VER > 1000)
#pragma once
#endif /* (_MSC_VER > 1000) */


#include "vislib/AbstractPointImpl.h"


namespace vislib {
namespace math {

    /**

     */
    template<class T, unsigned int D, class S> class AbstractPoint 
            : public AbstractPointImpl<T, D, S, AbstractPoint> {

    public:

        /** Dtor. */
        ~AbstractPoint(void);

    protected:

        /** Typedef for our super class. */
        typedef AbstractPointImpl<T, D, S, vislib::math::AbstractPoint> Super;

        /**
         * Disallow instances of this class. 
         */
        inline AbstractPoint(void) : Super() {}
    };


    /*
     * vislib::math::AbstractPoint<T, D, S>::~AbstractPoint
     */
    template<class T, unsigned int D, class S>
    AbstractPoint<T, D, S>::~AbstractPoint(void) {
    }


    /**
     * Partial template specialisation for two-dimensional points. This 
     * implementation provides convenience access methods to the two 
     * coordinates.
     */
    template<class T, class S> class AbstractPoint<T, 2, S> 
            : public AbstractPointImpl<T, 2, S, AbstractPoint> {

    public:

        /** Dtor. */
        ~AbstractPoint(void);

        /**
         * Answer the x-coordinate of the point.
         *
         * @return The x-coordinate of the point.
         */
        inline const T& GetX(void) const {
            return this->coordinates[0];
        }

        /**
         * Answer the y-coordinate of the point.
         *
         * @return The y-coordinate of the point.
         */
        inline const T& GetY(void) const {
            return this->coordinates[1];
        }

        /**
         * Set the coordinates ot the point.
         *
         * @param x The x-coordinate of the point.
         * @param y The y-coordinate of the point.
         */
        inline void Set(const T& x, const T& y) {
            this->coordinates[0] = x;
            this->coordinates[1] = y;
        }

        /**
         * Set the x-coordinate of the point.
         *
         * @param x The new x-coordinate.
         */
        inline void SetX(const T& x) {
            this->coordinates[0] = x;
        }

        /**
         * Set the y-coordinate of the point.
         *
         * @param y The new y-coordinate.
         */
        inline void SetY(const T& y) {
            this->coordinates[1] = y;
        }

        /**
         * Answer the x-coordinate of the point.
         *
         * @return The x-coordinate of the point.
         */
        inline const T& X(void) const {
            return this->coordinates[0];
        }

        /**
         * Answer the y-component of the point.
         *
         * @return The y-component of the point.
         */
        inline const T& Y(void) const {
            return this->coordinates[1];
        }

    protected:

        /** Typedef for our super class. */
        typedef AbstractPointImpl<T, 2, S, vislib::math::AbstractPoint> Super;

        /**
         * Disallow instances of this class. 
         */
        inline AbstractPoint(void) : Super() {}
    };


    /*
     * vislib::math::AbstractPoint<T, 2, S>::~AbstractPoint
     */
    template<class T, class S> AbstractPoint<T, 2, S>::~AbstractPoint(void) {
    }


    /**
     * Partial template specialisation for three-dimensional points. This 
     * implementation provides convenience access methods to the three 
     * coordinates.
     */
    template<class T, class S> class AbstractPoint<T, 3, S> 
            : public AbstractPointImpl<T, 3, S, AbstractPoint> {

    public:

        /** Dtor. */
        ~AbstractPoint(void);

        /**
         * Answer the x-coordinate of the point.
         *
         * @return The x-coordinate of the point.
         */
        inline const T& GetX(void) const {
            return this->coordinates[0];
        }

        /**
         * Answer the y-coordinate of the point.
         *
         * @return The y-coordinate of the point.
         */
        inline const T& GetY(void) const {
            return this->coordinates[1];
        }

        /**
         * Answer the z-coordinate of the point.
         *
         * @return The z-coordinate of the point.
         */
        inline const T& GetZ(void) const {
            return this->coordinates[2];
        }

        /**
         * Set the coordinates ot the point.
         *
         * @param x The x-coordinate of the point.
         * @param y The y-coordinate of the point.
         * @param z The z-coordinate of the point.
         */
        inline void Set(const T& x, const T& y, const T& z) {
            this->coordinates[0] = x;
            this->coordinates[1] = y;
            this->coordinates[2] = z;
        }

        /**
         * Set the x-coordinate of the point.
         *
         * @param x The new x-coordinate.
         */
        inline void SetX(const T& x) {
            this->coordinates[0] = x;
        }

        /**
         * Set the y-coordinate of the point.
         *
         * @param y The new y-coordinate.
         */
        inline void SetY(const T& y) {
            this->coordinates[1] = y;
        }

        /**
         * Set the z-coordinate of the point.
         *
         * @param z The new z-coordinate.
         */
        inline void SetZ(const T& z) {
            this->coordinates[2] = z;
        }

        /**
         * Answer the x-coordinate of the point.
         *
         * @return The x-coordinate of the point.
         */
        inline const T& X(void) const {
            return this->coordinates[0];
        }

        /**
         * Answer the y-coordinate of the point.
         *
         * @return The y-coordinate of the point.
         */
        inline const T& Y(void) const {
            return this->coordinates[1];
        }

        /**
         * Answer the z-coordinate of the point.
         *
         * @return The z-coordinate of the point.
         */
        inline const T& Z(void) const {
            return this->coordinates[2];
        }

    protected:

        /** Typedef for our super class. */
        typedef AbstractPointImpl<T, 3, S, vislib::math::AbstractPoint> Super;

        /**
         * Disallow instances of this class. 
         */
        inline AbstractPoint(void) : Super() {}
    };


    /*
     * vislib::math::AbstractPoint<T, 3, S>::~AbstractPoint
     */
    template<class T, class S> AbstractPoint<T, 3, S>::~AbstractPoint(void) {
    }

} /* end namespace math */
} /* end namespace vislib */

#endif /* VISLIB_ABSTRACTPOINT_H_INCLUDED */
