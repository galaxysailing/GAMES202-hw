class DirectionalLight {

    constructor(lightIntensity, lightColor, lightPos, focalPoint, lightUp, hasShadowMap, gl) {
        this.mesh = Mesh.cube(setTransform(0, 0, 0, 0.2, 0.2, 0.2, 0));
        this.mat = new EmissiveMaterial(lightIntensity, lightColor);
        this.lightPos = lightPos;
        this.focalPoint = focalPoint;
        this.lightUp = lightUp

        this.hasShadowMap = hasShadowMap;
        this.fbo = new FBO(gl);
        if (!this.fbo) {
            console.log("无法设置帧缓冲区对象");
            return;
        }
    }

    CalcLightMVP(translate, scale) {
        let lightMVP = mat4.create();
        let modelMatrix = mat4.create();
        let viewMatrix = mat4.create();
        let projectionMatrix = mat4.create();
        // console.log(mat4);
        // var k = width / height //窗口宽高比
        let s = 150;
        // let camera = new THREE.OrthographicCamera(-s, s, s, -s, 1, 1000)
        // camera.position.set(this.lightPos)
        // camera.lookAt(this.focalPoint);
        
        // Model transform
        mat4.translate(modelMatrix, modelMatrix, translate);
        mat4.scale(modelMatrix, modelMatrix, scale);
        // View transform
        //[0, 80, 80]
        mat4.lookAt(viewMatrix, this.lightPos, this.focalPoint, this.lightUp);
        // // Projection transform
        mat4.ortho(projectionMatrix, -s, s, -s, s, 1, 1000);
        
        mat4.multiply(lightMVP, projectionMatrix, viewMatrix);
        mat4.multiply(lightMVP, lightMVP, modelMatrix);
        // console.log(lightMVP);
        return lightMVP;
    }
}
