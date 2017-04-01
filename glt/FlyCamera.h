//
//  FlyCamera.h
//  glt
//
//  Created by Mario Hros on 2. 1. 14.
//  Copyright (c) 2014 K3A. All rights reserved.
//

#ifndef __glt__FlyCamera__
#define __glt__FlyCamera__

#include "vec3.hpp"
#include "mat4x4.hpp"

/**
 A first-person shooter type of camera.
 
 Set the properties of the camera, then use the `matrix` method to get the camera matrix for
 use in the vertex shader.
 
 Includes the perspective projection matrix.
 */
class CFlyCamera {
public:
    CFlyCamera();
    
    /**
     The position of the camera.
     */
    const glm::vec3& GetPosition() const;
    void SetPosition(const glm::vec3& position);
    void OffsetPosition(const glm::vec3& offset);
    
    /**
     The vertical viewing angle of the camera, in degrees.
     
     Determines how "wide" the view of the camera is. Large angles appear to be zoomed out,
     as the camera has a wide view. Small values appear to be zoomed in, as the camera has a
     very narrow view.
     
     The value must be between 0 and 180.
     */
    float GetFieldOfView() const;
    void SetFieldOfView(float fieldOfView);
    
    /**
     The closest visible distance from the camera.
     
     Objects that are closer to the camera than the near plane distance will not be visible.
     
     Value must be greater than 0.
     */
    float GetNearPlane() const;
    
    /**
     The farthest visible distance from the camera.
     
     Objects that are further away from the than the far plane distance will not be visible.
     
     Value must be greater than the near plane
     */
    float GetFarPlane() const;
    
    /**
     Sets the near and far plane distances.
     
     Everything between the near plane and the var plane will be visible. Everything closer
     than the near plane, or farther than the far plane, will not be visible.
     
     @param nearPlane  Minimum visible distance from camera. Must be > 0
     @param farPlane   Maximum visible distance from vamera. Must be > nearPlane
     */
    void SetNearAndFarPlanes(float nearPlane, float farPlane);
    
    /**
     A rotation matrix that determines the direction the camera is looking.
     
     Does not include translation (the camera's position).
     */
    glm::mat4 GetOrientation() const;
    
    /**
     Offsets the cameras orientation.
     
     The verticle angle is constrained between 85deg and -85deg to avoid gimbal lock.
     
     @param upAngle     the angle (in degrees) to offset upwards. Negative values are downwards.
     @param rightAngle  the angle (in degrees) to offset rightwards. Negative values are leftwards.
     */
    void OffsetOrientation(float rightAngle, float upAngle);
    
    /**
     Orients the camera so that is it directly facing `position`
     
     @param position  the position to look at
     */
    void LookAt(glm::vec3 position);
    
    /**
     The width divided by the height of the screen/window/viewport
     
     Incorrect values will make the 3D scene look stretched.
     */
    float GetViewportAspectRatio() const;
    void SetViewportAspectRatio(float viewportAspectRatio);
    
    /** A unit vector representing the direction the camera is facing */
    glm::vec3 GetForward() const;
    
    /** A unit vector representing the direction to the right of the camera*/
    glm::vec3 GetRight() const;
    
    /** A unit vector representing the direction out of the top of the camera*/
    glm::vec3 GetUp() const;
    
    /**
     The combined camera transformation matrix, including perspective projection.
     
     This is the complete matrix to use in the vertex shader.
     */
    glm::mat4 GetMatrix() const;
    
    /**
     The perspective projection transformation matrix
     */
    glm::mat4 GetProjection() const;
    
    /**
     The translation and rotation matrix of the camera.
     
     Same as the `matrix` method, except the return value does not include the projection
     transformation.
     */
    glm::mat4 GetView() const;
    
private:
    glm::vec3 _position;
    float _horizontalAngle;
    float _verticalAngle;
    float _fieldOfView;
    float _nearPlane;
    float _farPlane;
    float _viewportAspectRatio;
    
    void NormalizeAngles();
};


#endif /* defined(__glt__FlyCamera__) */
