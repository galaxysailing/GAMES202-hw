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
    int num = kernelRadius * 2 + 1;
    num *= num;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Temporal clamp
            // Done
            Float3 color = m_accColor(x, y);
            //Float3 mu(0.0f);
            //Float3 sigma(0.0f);
            //int cnt = 0;
            //for (int i = -kernelRadius; i <= kernelRadius; ++i) {
            //    for (int j = -kernelRadius; j <= kernelRadius; ++j) {
            //        int nx = x + i, ny = y + j;
            //        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            //            mu += m_accColor(nx, ny);
            //        }
            //    }
            //}
            //mu /= num;
            //for (int i = -kernelRadius; i <= kernelRadius; ++i) {
            //    for (int j = -kernelRadius; j <= kernelRadius; ++j) {
            //        int nx = x + i, ny = y + j;
            //        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            //            Float3 c = m_accColor(nx, ny);
            //            Float3 t = c - mu;
            //            t = t * t;
            //            sigma += t;
            //        }
            //    }
            //}
            //sigma /= num;
            // TODO: Exponential moving average
            // Done
            if (!m_valid(x, y)) {
                float alpha = 1.0f;
                m_misc(x, y) = Lerp(color, curFilteredColor(x, y), alpha);
            } else {
                //Float3 kdotsigma = sigma * m_colorBoxK;
                //color = Clamp(color, mu - kdotsigma, mu + kdotsigma);
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
    int kernelRadius = 16;
#pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // TODO: Joint bilateral filter
            //filteredImage(x, y) = frameInfo.m_beauty(x, y);
            
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
