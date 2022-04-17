function getRotationPrecomputeL(precompute_L, rotationMatrix){
	let m1 = computeSquareMatrix_3by3(rotationMatrix);
	let m2 = computeSquareMatrix_5by5(rotationMatrix);
	var t_L = new Array(9);
	for(let i = 0; i < 9; ++i){
		t_L[i] = new Array(3);
	}
	
	// precompute_L 9 * 3
	for(let i = 0; i < 3; ++i){
		let v1 = [
			precompute_L[1][i],
			precompute_L[2][i],
			precompute_L[3][i]
		]
		let res1 = math.multiply(m1, v1);
		// console.log(res1);
		t_L[1][i] = res1[0];
		t_L[2][i] = res1[1];
		t_L[3][i] = res1[2];

		let v2 = [
			precompute_L[4][i],
			precompute_L[5][i],
			precompute_L[6][i],
			precompute_L[7][i],
			precompute_L[8][i],
		]
		let res2 = math.multiply(m2, v2);
		t_L[4][i] = res2[0];
		t_L[5][i] = res2[1];
		t_L[6][i] = res2[2];
		t_L[7][i] = res2[3];
		t_L[8][i] = res2[4];

	}
	t_L[0][0] = precompute_L[0][0];
	t_L[0][1] = precompute_L[0][1];
	t_L[0][2] = precompute_L[0][2];
	
	return t_L;
}

function computeSquareMatrix_3by3(rotationMatrix){ // 计算方阵SA(-1) 3*3 

	// 1、pick ni - {ni}
	let n1 = [1, 0, 0, 0]; let n2 = [0, 0, 1, 0]; let n3 = [0, 1, 0, 0];
	let order3_1 = SHEval3(n1[0], n1[1], n1[2]);
	let order3_2 = SHEval3(n2[0], n2[1], n2[2]);
	let order3_3 = SHEval3(n3[0], n3[1], n3[2]);
	// 2、{P(ni)} - A  A_inverse
	let A = [
		[order3_1[1], order3_2[1], order3_3[1]],
		[order3_1[2], order3_2[2], order3_3[2]],
		[order3_1[3], order3_2[3], order3_3[3]]
	]
	let A_inv = math.inv(A);

	// 3、用 R 旋转 ni - {R(ni)}
	
	rotMatrix = mat4Matrix2mathMatrix(rotationMatrix);
	let rn1 = math.multiply(rotMatrix, n1)._data;
	let rn2 = math.multiply(rotMatrix, n2)._data;
	let rn3 = math.multiply(rotMatrix, n3)._data;
	// 4、R(ni) SH投影 - S
	let r_order3_1 = SHEval3(rn1[0], rn1[1], rn1[2]);
	let r_order3_2 = SHEval3(rn2[0], rn2[1], rn2[2]);
	let r_order3_3 = SHEval3(rn3[0], rn3[1], rn3[2]);
	let S = [
		[r_order3_1[1], r_order3_2[1], r_order3_3[1]],
		[r_order3_1[2], r_order3_2[2], r_order3_3[2]],
		[r_order3_1[3], r_order3_2[3], r_order3_3[3]]
	]
	// console.log(A_inv);
	// 5、S*A_inverse
	return math.multiply(S, A_inv);

}

function computeSquareMatrix_5by5(rotationMatrix){ // 计算方阵SA(-1) 5*5
	
	// 1、pick ni - {ni}
	let k = 1 / math.sqrt(2);
	let n1 = [1, 0, 0, 0]; let n2 = [0, 0, 1, 0]; let n3 = [k, k, 0, 0]; 
	let n4 = [k, 0, k, 0]; let n5 = [0, k, k, 0];
	let order3_1 = SHEval3(n1[0], n1[1], n1[2]);
	let order3_2 = SHEval3(n2[0], n2[1], n2[2]);
	let order3_3 = SHEval3(n3[0], n3[1], n3[2]);
	let order3_4 = SHEval3(n4[0], n4[1], n4[2]);
	let order3_5 = SHEval3(n5[0], n5[1], n5[2]);

	rotMatrix = mat4Matrix2mathMatrix(rotationMatrix);

	// 2、{P(ni)} - A  A_inverse
	let A = [
		[order3_1[4], order3_2[4], order3_3[4], order3_4[4], order3_5[4]],
		[order3_1[5], order3_2[5], order3_3[5], order3_4[5], order3_5[5]],
		[order3_1[6], order3_2[6], order3_3[6], order3_4[6], order3_5[6]],
		[order3_1[7], order3_2[7], order3_3[7], order3_4[7], order3_5[7]],
		[order3_1[8], order3_2[8], order3_3[8], order3_4[8], order3_5[8]]
	]
	let A_inv = math.inv(A);

	// 3、用 R 旋转 ni - {R(ni)}
	let rn1 = math.multiply(rotMatrix, n1)._data;
	let rn2 = math.multiply(rotMatrix, n2)._data;
	let rn3 = math.multiply(rotMatrix, n3)._data;
	let rn4 = math.multiply(rotMatrix, n4)._data;
	let rn5 = math.multiply(rotMatrix, n5)._data;

	// 4、R(ni) SH投影 - S
	let r_order3_1 = SHEval3(rn1[0], rn1[1], rn1[2]);
	let r_order3_2 = SHEval3(rn2[0], rn2[1], rn2[2]);
	let r_order3_3 = SHEval3(rn3[0], rn3[1], rn3[2]);
	let r_order3_4 = SHEval3(rn4[0], rn4[1], rn4[2]);
	let r_order3_5 = SHEval3(rn5[0], rn5[1], rn5[2]);
	let S = [
		[r_order3_1[4], r_order3_2[4], r_order3_3[4], r_order3_4[4], r_order3_5[4]],
		[r_order3_1[5], r_order3_2[5], r_order3_3[5], r_order3_4[5], r_order3_5[5]],
		[r_order3_1[6], r_order3_2[6], r_order3_3[6], r_order3_4[6], r_order3_5[6]],
		[r_order3_1[7], r_order3_2[7], r_order3_3[7], r_order3_4[7], r_order3_5[7]],
		[r_order3_1[8], r_order3_2[8], r_order3_3[8], r_order3_4[8], r_order3_5[8]]
	]

	// 5、S*A_inverse
	return math.multiply(S, A_inv);

}

function mat4Matrix2mathMatrix(rotationMatrix){

	let mathMatrix = [];
	for(let i = 0; i < 4; i++){
		let r = [];
		for(let j = 0; j < 4; j++){
			r.push(rotationMatrix[i*4+j]);
		}
		mathMatrix.push(r);
	}
	return math.matrix(mathMatrix)

}

function getMat3ValueFromRGB(precomputeL){

    let colorMat3 = [];
    for(var i = 0; i<3; i++){
        colorMat3[i] = mat3.fromValues( precomputeL[0][i], precomputeL[1][i], precomputeL[2][i],
										precomputeL[3][i], precomputeL[4][i], precomputeL[5][i],
										precomputeL[6][i], precomputeL[7][i], precomputeL[8][i] ); 
	}
    return colorMat3;
}

function angle2radius(angle){
	return math.tau / 360 * angle;
}