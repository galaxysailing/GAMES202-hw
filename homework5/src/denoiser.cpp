#include "denoiser.h"

Denoiser::Denoiser() : m_useTemportal(false) {}

void Denoiser::Reprojection(const FrameInfo &frameInfo) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    Matrix4x4 preWorldToScreen =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 1];
    Matrix4x4 preWorldToCamera =
        m_preFrameInfo.m_matrix[m_preFrameInfo.m_matrix.size() - 2];

#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Reproject
            m_valid(x, y) = false;
            m_misc(x, y) = Float3(0.f);

            int id = frameInfo.m_id(x, y);
            if (id == -1) {
                continue;
            }
            const Matrix4x4& objToWorld = frameInfo.m_matrix[id];
            const Matrix4x4 &worldToObj = Inverse(objToWorld);
            Float3 pos = frameInfo.m_position(x, y);

            const Matrix4x4 &preObjToWorld = m_preFrameInfo.m_matrix[id];
            Float3 preScreenPos = preWorldToScreen(preObjToWorld(worldToObj(pos, Float3::EType::Point), Float3::EType::Point), Float3::EType::Point);
            if (preScreenPos.x >= 0 && preScreenPos.x < width && preScreenPos.y >= 0 && preScreenPos.y < height) {
                int preId = m_preFrameInfo.m_id(preScreenPos.x, preScreenPos.y);
                if (preId == id) {
                    m_misc(x, y) = m_accColor(preScreenPos.x, preScreenPos.y);
                    m_valid(x, y) = true;
                }
            }
        }
    }
    std::swap(m_misc, m_accColor);
}

void Denoiser::TemporalAccumulation(const Buffer2D<Float3> &curFilteredColor) {
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    int kernelRadius = 3;
    float num = kernelRadius * 2 + 1;
    num = num * num;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Temporal clamp
            // Done
            Float3 color = m_accColor(x, y);
            Float3 mu(0.0f);
            Float3 sigma(0.0f);
            for (int i = -kernelRadius; i <= kernelRadius; ++i) {
                for (int j = -kernelRadius; j <= kernelRadius; ++j) {
                    int nx = x + i, ny = y + j;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        mu += curFilteredColor(nx, ny);
                    }
                }
            }
            mu /= num;
            for (int i = -kernelRadius; i <= kernelRadius; ++i) {
                for (int j = -kernelRadius; j <= kernelRadius; ++j) {
                    int nx = x + i, ny = y + j;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        Float3 c = curFilteredColor(nx, ny);
                        Float3 t = c - mu;
                        sigma += Sqr(t);
                    }
                }
            }
            sigma /= num;
            sigma = SafeSqrt(sigma);
            // TODO: Exponential moving average
            // Done
            if (!m_valid(x, y)) {
                float alpha = 1.0f;
                m_misc(x, y) = Lerp(color, curFilteredColor(x, y), alpha);
            } else {
                Float3 kdotsigma = sigma * m_colorBoxK;
                color = Clamp(color, mu - kdotsigma, mu + kdotsigma);
                m_misc(x, y) = Lerp(color, curFilteredColor(x, y), m_alpha);
            }
        }
    }
    std::swap(m_misc, m_accColor);
}
Buffer2D<Float3> Denoiser::Filter(const FrameInfo &frameInfo) {
    int height = frameInfo.m_beauty.m_height;
    int width = frameInfo.m_beauty.m_width;
    Buffer2D<Float3> filteredImage = CreateBuffer2D<Float3>(width, height);
#define A_TROUS_WAVELET
#ifdef A_TROUS_WAVELET
    const static float a_trous_kernel[25] = {
        1 / 256.0, 1 / 64.0, 3 / 128.0, 1 / 64.0, 1 / 256.0,
        1 / 64.0, 1 / 16.0, 3 / 32.0, 1 / 16.0, 1 / 64.0,
        3 / 128.0, 3 / 32.0, 9 / 64.0, 3 / 32.0, 3 / 128.0,
        1 / 64.0, 1 / 16.0, 3 / 32.0, 1 / 16.0, 1 / 64.0,
        1 / 256.0, 1 / 64.0, 3 / 128.0, 1 / 64.0, 1 / 256.0};
    float stepWidth = 1.0f;
    const int kernelRadius = 2;
    const float c_phi = 1.0f, n_phi = 0.01f, p_phi = 0.1f;
    Buffer2D<Float3> dstTmpImage = CreateBuffer2D<Float3>(width, height);
    auto *src_color_map = &filteredImage;
    auto *dst_color_map = &dstTmpImage;
    for (int it = 0; it < 5; ++it) {
#pragma omp parallel for
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Float3 cval = (*src_color_map)(x, y);
                Float3 nval = frameInfo.m_normal(x, y);
                Float3 pval = frameInfo.m_position(x, y);
                Float3 sum(0.f);
                float cum_w = 0.0;
                for (int i = -kernelRadius; i <= kernelRadius; ++i) {
                    for (int j = -kernelRadius; j <= kernelRadius; ++j) {
                        int nx = x + i * stepWidth, ny = y + j * stepWidth;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            Float3 ctmp = it == 0?frameInfo.m_beauty(nx,ny):(*src_color_map)(nx, ny);
                            Float3 t = cval - ctmp;
                            float dist2 = Dot(t, t);
                            float c_w = std::min(exp(-(dist2) / c_phi), 1.0f);

                            Float3 ntmp = frameInfo.m_normal(nx, ny);
                            t = nval - ntmp;
                            dist2 = std::max(Dot(t, t) / (stepWidth * stepWidth), 0.0f);
                            float n_w = std::min(exp(-(dist2) / n_phi), 1.0f);

                            Float3 ptmp = frameInfo.m_position(nx, ny);
                            t = pval - ptmp;
                            dist2 = Dot(t, t);
                            float p_w = std::min(exp(-(dist2) / p_phi), 1.0f);

                            float weight = c_w * n_w * p_w * a_trous_kernel[(i + kernelRadius * 5) + j + kernelRadius];
                            //weight /= stepWidth;
                            sum += ctmp * weight;
                            cum_w += weight;
                        }
                    }
                }
                if (cum_w != 0.0) {
                    (*dst_color_map)(x, y) = sum / cum_w;
                }
            }
        }

        stepWidth *= 2.0f;
        auto t = src_color_map;
        src_color_map = dst_color_map;
        dst_color_map = t;
    }
    return *dst_color_map;
#else
    int kernelRadius = 16;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Joint bilateral filter
            //filteredImage(x, y) = frameInfo.m_beauty(x, y);
            // Done
            Float3 color = Float3(0.0);
            float weight = 0.0;
            for (int i = -kernelRadius; i < kernelRadius; ++i) {
                for (int j = -kernelRadius; j <= kernelRadius; ++j) {
                    int nx = x + i, ny = y + j;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        float item_pos = SqrLength(frameInfo.m_position(x, y) -
                                                frameInfo.m_position(nx, ny)) /
                                         (2.0f * m_sigmaPlane * m_sigmaPlane);
                        float item_color = SqrLength(frameInfo.m_beauty(x, y) -
                                                frameInfo.m_beauty(nx, ny)) /
                                           (2.0f * m_sigmaColor * m_sigmaColor);
                        float item_normal = acos(std::clamp(Dot(frameInfo.m_normal(x, y),
                                                     frameInfo.m_normal(nx, ny)), -1.0f, 1.0f));
                        item_normal *= item_normal;
                        item_normal /= 2.0f * m_sigmaNormal * m_sigmaNormal;
                        Float3 vt = frameInfo.m_position(nx, ny) - frameInfo.m_position(x, y);
                        float item_plane = 0.0;
                        if (Length(vt) != 0.0) {
                            item_plane = Dot(frameInfo.m_normal(x, y), Normalize(vt));
                            item_plane *= item_plane;
                            item_plane /= 2.0f * m_sigmaCoord * m_sigmaCoord;
                        }
                        float J = exp(-(item_pos + item_color + item_normal + item_plane));
                        weight += J;
                        color += frameInfo.m_beauty(nx, ny) * J;
                    }
                }
            }
            if (weight != 0.0f) {
                filteredImage(x, y) = color / weight;
            }
        }
    }
    return filteredImage;
#endif
}

void Denoiser::Init(const FrameInfo &frameInfo, const Buffer2D<Float3> &filteredColor) {
    m_accColor.Copy(filteredColor);
    int height = m_accColor.m_height;
    int width = m_accColor.m_width;
    m_misc = CreateBuffer2D<Float3>(width, height);
    m_valid = CreateBuffer2D<bool>(width, height);
}

void Denoiser::Maintain(const FrameInfo &frameInfo) { m_preFrameInfo = frameInfo; }

Buffer2D<Float3> Denoiser::ProcessFrame(const FrameInfo &frameInfo) {
    // Filter current frame
    Buffer2D<Float3> filteredColor;
    filteredColor = Filter(frameInfo);

    // Reproject previous frame color to current
    if (m_useTemportal) {
        Reprojection(frameInfo);
        TemporalAccumulation(filteredColor);
    } else {
        Init(frameInfo, filteredColor);
    }

    // Maintain
    Maintain(frameInfo);
    if (!m_useTemportal) {
        m_useTemportal = true;
    }
    return m_accColor;
}
