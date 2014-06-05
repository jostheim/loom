#pragma once

#include <loom/cross_cat.hpp>

namespace loom
{

class PredictServer
{
public:

    typedef protobuf::Post::Sample::Query Query;
    typedef protobuf::Post::Sample::Result Result;
    typedef CrossCat::Value Value;

    PredictServer (const CrossCat & cross_cat) :
        cross_cat_(cross_cat),
        value_join_(cross_cat),
        to_predict_(),
        partial_values_(),
        result_factors_(),
        scores_(),
        timer_()
    {
    }

    void predict_row (
            rng_t & rng,
            const Query & query,
            Result & result);

private:

    const CrossCat & cross_cat_;
    CrossCat::ValueJoiner value_join_;
    Value to_predict_;
    std::vector<Value> partial_values_;
    std::vector<std::vector<Value>> result_factors_;
    VectorFloat scores_;
    Timer timer_;
};

inline void PredictServer::predict_row (
        rng_t & rng,
        const Query & query,
        Result & result)
{
    Timer::Scope timer(timer_);

    result.Clear();
    result.set_id(query.id());
    if (not cross_cat_.schema.is_valid(query.data())) {
        result.set_error("invalid query data");
        return;
    }
    if (query.data().observed_size() != query.to_predict_size()) {
        result.set_error("observed size != to_predict size");
        return;
    }
    const size_t sample_count = query.sample_count();
    if (sample_count == 0) {
        return;
    }

    cross_cat_.value_split(query.data(), partial_values_);
    * to_predict_.mutable_observed() = query.to_predict();
    result_factors_.resize(sample_count);
    cross_cat_.value_split_observed(to_predict_, result_factors_.front());
    std::fill(
        result_factors_.begin() + 1,
        result_factors_.end(),
        result_factors_.front());

    const size_t kind_count = cross_cat_.kinds.size();
    for (size_t i = 0; i < kind_count; ++i) {
        if (cross_cat_.schema.observed_count(result_factors_.front()[i])) {
            const Value & value = partial_values_[i];
            auto & kind = cross_cat_.kinds[i];
            const ProductModel & model = kind.model;
            auto & mixture = kind.mixture;

            mixture.score_value(model, value, scores_, rng);
            distributions::scores_to_probs(scores_);
            const VectorFloat & probs = scores_;

            for (auto & result_values : result_factors_) {
                mixture.sample_value(model, probs, result_values[i], rng);
            }
        }
    }

    for (const auto & result_values : result_factors_) {
        value_join_(* result.add_samples(), result_values);
    }
}

} // namespace loom
