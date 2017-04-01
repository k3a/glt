//
//  FlyCamera.cpp
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

// Taken from https://github.com/tomdalling/opengl-series/blob/master/osx/04_camera/source/tdogl/Camera.cpp

#include "FlyCamera.h"

#include "func_matrix.hpp"
#include "matrix_transform.hpp"
#include <cmath>


static const float MaxVerticalAngle = 85.0f; //must be less than 90 to avoid gimbal lock

static inline float RadiansToDegrees(float radians)
{
    return radians * 180.0f / (float)3.14159265358979323846;
}

CFlyCamera::CFlyCamera() :
_position(0.0f, 0.0f, 0.0f),
_horizontalAngle(0.0f),
_verticalAngle(0.0f),
_fieldOfView(50.0f),
_nearPlane(0.01f),
_farPlane(100.0f),
_viewportAspectRatio(4.0f/3.0f)
{}

const glm::vec3& CFlyCamera::GetPosition() const
{
    return _position;
}

void CFlyCamera::SetPosition(const glm::vec3& position)
{
    _position = position;
}

void CFlyCamera::OffsetPosition(const glm::vec3& offset)
{
    _position += offset;
}

float CFlyCamera::GetFieldOfView() const
{
    return _fieldOfView;
}

void CFlyCamera::SetFieldOfView(float fieldOfView)
{
    assert(fieldOfView > 0.0f && fieldOfView < 180.0f);
    _fieldOfView = fieldOfView;
}

float CFlyCamera::GetNearPlane() const
{
    return _nearPlane;
}

float CFlyCamera::GetFarPlane() const
{
    return _farPlane;
}

void CFlyCamera::SetNearAndFarPlanes(float nearPlane, float farPlane)
{
    assert(nearPlane > 0.0f);
    assert(farPlane > nearPlane);
    _nearPlane = nearPlane;
    _farPlane = farPlane;
}

glm::mat4 CFlyCamera::GetOrientation() const
{
    glm::mat4 orientation;
    orientation = glm::rotate(orientation, _verticalAngle, glm::vec3(1,0,0));
    orientation = glm::rotate(orientation, _horizontalAngle, glm::vec3(0,1,0));
    return orientation;
}

void CFlyCamera::OffsetOrientation(float rightAngle, float upAngle)
{
    _horizontalAngle += rightAngle;
    _verticalAngle += upAngle;
    NormalizeAngles();
}

void CFlyCamera::LookAt(glm::vec3 position)
{
    assert(position != _position);
    glm::vec3 direction = glm::normalize(position - _position);
    _verticalAngle = RadiansToDegrees(asinf(-direction.y));
    _horizontalAngle = -RadiansToDegrees(atan2f(-direction.x, -direction.z));
    NormalizeAngles();
}

float CFlyCamera::GetViewportAspectRatio() const
{
    return _viewportAspectRatio;
}

void CFlyCamera::SetViewportAspectRatio(float viewportAspectRatio)
{
    assert(viewportAspectRatio > 0.0);
    _viewportAspectRatio = viewportAspectRatio;
}

glm::vec3 CFlyCamera::GetForward() const
{
    glm::vec4 forward = glm::inverse(GetOrientation()) * glm::vec4(0,0,-1,1);
    return glm::vec3(forward);
}

glm::vec3 CFlyCamera::GetRight() const
{
    glm::vec4 right = glm::inverse(GetOrientation()) * glm::vec4(1,0,0,1);
    return glm::vec3(right);
}

glm::vec3 CFlyCamera::GetUp() const
{
    glm::vec4 up = glm::inverse(GetOrientation()) * glm::vec4(0,1,0,1);
    return glm::vec3(up);
}

glm::mat4 CFlyCamera::GetMatrix() const
{
    return GetProjection() * GetView();
}

glm::mat4 CFlyCamera::GetProjection() const
{
    return glm::perspective(_fieldOfView, _viewportAspectRatio, _nearPlane, _farPlane);
}

glm::mat4 CFlyCamera::GetView() const
{
    return GetOrientation() * glm::translate(glm::mat4(), -_position);
}

void CFlyCamera::NormalizeAngles()
{
    _horizontalAngle = fmodf(_horizontalAngle, 360.0f);
    //fmodf can return negative values, but this will make them all positive
    if(_horizontalAngle < 0.0f)
        _horizontalAngle += 360.0f;
    
    if(_verticalAngle > MaxVerticalAngle)
        _verticalAngle = MaxVerticalAngle;
    else if(_verticalAngle < -MaxVerticalAngle)
        _verticalAngle = -MaxVerticalAngle;
}



