/**
 * Copyright (c) 2018, University Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University Osnabrück nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL University Osnabrück BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * LVRPickingInteractor.hpp
 *
 *  @date Feb 19, 2014
 *  @author Thomas Wiemann
 */
#ifndef LVRPICKINGINTERACTOR_HPP_
#define LVRPICKINGINTERACTOR_HPP_

#include <QObject>

#include <vtkTextActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>

namespace lvr2
{

enum PickMode {None, PickPoint, PickFirst, PickSecond};

class LVRPickingInteractor : public QObject, public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT
public:
    LVRPickingInteractor(vtkSmartPointer<vtkRenderer> renderer);
    virtual ~LVRPickingInteractor();

    /**
     * @brief   Overloaded mouse event handling.
     */
    virtual void OnLeftButtonDown();

    /**
     * @brief   Overloaded keyboard press event handling
     */
    virtual void OnKeyPress();

    /**
     * @brief   Overloaded keyboard release event handling
     */
    virtual void OnKeyRelease();

    virtual void OnKeyDown();

    virtual void Dolly();
    virtual void Pan();
    virtual void Spin();
    virtual void Zoom();
    virtual void Rotate();

    /**
     * @brief   returns the text-actor, needed to readd-it after clearing the render window
     */
    vtkSmartPointer<vtkTextActor>   getTextActor(){ return m_textActor; }

public Q_SLOTS:
    void correspondenceSearchOn();
    void correspondenceSearchOff();

Q_SIGNALS:
    void firstPointPicked(double*);
    void secondPointPicked(double*);
    void pointSelected(vtkActor*, int);

private:
    /// Indicates picking mode
    PickMode            m_pickMode;

    /// Text actor to display info if in picking mode
    vtkSmartPointer<vtkTextActor>   m_textActor;

    vtkSmartPointer<vtkRenderer>    m_renderer;

    bool                            m_correspondenceMode;

    unsigned int                    m_numberOfClicks;
    int                             m_previousPosition[2];

};

} /* namespace lvr2 */

#endif /* LVRPICKINGINTERACTOR_HPP_ */