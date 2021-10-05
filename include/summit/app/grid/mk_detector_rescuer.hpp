#pragma once
namespace summit::app::grid {

constexpr struct MkDetectorRescurer {
    template<class cvMatT>
    auto operator()(
        cv::Mat                    mat,
        const cv::Mat_<cvMatT>&    templ,
        const cv::Mat_<cvMatT>&    mask,
        std::vector<cv::Point2d>   mk_pos_spec,
        cv::Mat                    ref_warp_mat,
        double                     highP_cover_extend_r
    ) const {

        auto [bias, score] = chipimgproc::marker::detection::estimate_bias(
            mat, templ, mask, mk_pos_spec, ref_warp_mat, 
            false, cv::Size(0, 0), highP_cover_extend_r
        );
        auto warp_mat = ref_warp_mat.clone();
        {
            auto _bias = bias;
            chipimgproc::typed_mat(warp_mat, [&_bias](auto& mat){
                mat(0, 2) += _bias.x;
                mat(1, 2) += _bias.y;
            });
        }

        return std::make_tuple(
            warp_mat, score
        );
    }

} mk_detector_rescuer;

}