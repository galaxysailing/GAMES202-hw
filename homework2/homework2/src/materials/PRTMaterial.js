class PRTMaterial extends Material {

    constructor(vertexShader, fragmentShader) {
        let PL = precomputeL[guiParams.envmapId];
        super({
            'uPrecomputeL0': { type: '3fv', value: PL[0] },
            'uPrecomputeL1': { type: '3fv', value: PL[1] },
            'uPrecomputeL2': { type: '3fv', value: PL[2] },
            'uPrecomputeL3': { type: '3fv', value: PL[3] },
            'uPrecomputeL4': { type: '3fv', value: PL[4] },
            'uPrecomputeL5': { type: '3fv', value: PL[5] },
            'uPrecomputeL6': { type: '3fv', value: PL[6] },
            'uPrecomputeL7': { type: '3fv', value: PL[7] },
            'uPrecomputeL8': { type: '3fv', value: PL[8] },
            'uLightness': { type: '1f', value: guiParams.lightness },
        }, [
            'aPrecomputeLT'
        ], vertexShader, fragmentShader, null);
    }
}

async function buildPRTMaterial(vertexPath, fragmentPath) {

    let vertexShader = await getShaderString(vertexPath);
    let fragmentShader = await getShaderString(fragmentPath);

    return new PRTMaterial(vertexShader, fragmentShader);

}