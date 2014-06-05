#pragma once

#include <type_traits>
#include <distributions/mixture.hpp>
#include <distributions/clustering.hpp>
#include <distributions/models/bb.hpp>
#include <distributions/models/dd.hpp>
#include <distributions/models/dpd.hpp>
#include <distributions/models/nich.hpp>
#include <distributions/models/gp.hpp>

namespace loom
{

//----------------------------------------------------------------------------
// Generics

template<class Model>
struct BaseModel
{
    template<bool cached>
    struct Mixture
    {
        typedef typename std::conditional<
            cached,
            typename Model::CachedMixture,
            typename Model::SimpleMixture>::type t;
    };

    static Model * null ()
    {
        return static_cast<Model *>(nullptr);
    }
};

//----------------------------------------------------------------------------
// Models

struct Clustering : BaseModel<Clustering>
{
    typedef typename distributions::Clustering<int>::PitmanYor Model;
    typedef Model Shared;
    typedef Model::Mixture CachedMixture;
    typedef distributions::MixtureDriver<Model, int> SimpleMixture;
};

struct BetaBernoulli : BaseModel<BetaBernoulli>
{
    typedef distributions::BetaBernoulli Model;
    typedef Model::Value Value;
    typedef Model::Shared Shared;
    typedef Model::Group Group;
    typedef Model::Sampler Sampler;
    typedef Model::Mixture CachedMixture;
    typedef distributions::MixtureSlave<Shared> SimpleMixture;
};

template<int max_dim>
struct DirichletDiscrete : BaseModel<DirichletDiscrete<max_dim>>
{
    typedef distributions::DirichletDiscrete<max_dim> Model;
    typedef typename Model::Value Value;
    typedef typename Model::Shared Shared;
    typedef typename Model::Group Group;
    typedef typename Model::Sampler Sampler;
    typedef typename Model::Mixture CachedMixture;
    typedef distributions::MixtureSlave<Shared> SimpleMixture;
};

struct DirichletProcessDiscrete : BaseModel<DirichletProcessDiscrete>
{
    typedef distributions::DirichletProcessDiscrete Model;
    typedef Model::Value Value;
    typedef Model::Shared Shared;
    typedef Model::Group Group;
    typedef Model::Sampler Sampler;
    typedef Model::Mixture CachedMixture;
    typedef distributions::MixtureSlave<Shared> SimpleMixture;
};

struct GammaPoisson : BaseModel<GammaPoisson>
{
    typedef distributions::GammaPoisson Model;
    typedef Model::Value Value;
    typedef Model::Shared Shared;
    typedef Model::Group Group;
    typedef Model::Sampler Sampler;
    typedef Model::Mixture CachedMixture;
    typedef distributions::MixtureSlave<Shared> SimpleMixture;
};

struct NormalInverseChiSq : BaseModel<NormalInverseChiSq>
{
    typedef distributions::NormalInverseChiSq Model;
    typedef Model::Value Value;
    typedef Model::Shared Shared;
    typedef Model::Group Group;
    typedef Model::Sampler Sampler;
    typedef Model::Mixture CachedMixture;
    typedef distributions::MixtureSlave<Shared> SimpleMixture;
};

//----------------------------------------------------------------------------
// Feature types

typedef BetaBernoulli BB;
typedef DirichletDiscrete<16> DD16;
typedef DirichletDiscrete<256> DD256;
typedef DirichletProcessDiscrete DPD;
typedef GammaPoisson GP;
typedef NormalInverseChiSq NICH;

template<class Fun>
inline void for_each_feature_type (Fun & fun)
{
    fun(BB::null());
    fun(DD16::null());
    fun(DD256::null());
    fun(DPD::null());
    fun(GP::null());
    fun(NICH::null());
}

template<class Fun>
inline bool for_some_feature_type (Fun & fun)
{
    return fun(BB::null())
        or fun(DD16::null())
        or fun(DD256::null())
        or fun(DPD::null())
        or fun(GP::null())
        or fun(NICH::null());
}

template<class Derived>
class ForEachFeatureType
{
    typedef typename Derived::template Container<BB>::t BBs;
    typedef typename Derived::template Container<DD16>::t DD16s;
    typedef typename Derived::template Container<DD256>::t DD256s;
    typedef typename Derived::template Container<DPD>::t DPDs;
    typedef typename Derived::template Container<GP>::t GPs;
    typedef typename Derived::template Container<NICH>::t NICHs;

public:

    BBs bb;
    DD16s dd16;
    DD256s dd256;
    DPDs dpd;
    GPs gp;
    NICHs nich;

    BBs & operator[] (BB *) { return bb; }
    DD16s & operator[] (DD16 *) { return dd16; }
    DD256s & operator[] (DD256 *) { return dd256; }
    DPDs & operator[] (DPD *) { return dpd; }
    GPs & operator[] (GP *) { return gp; }
    NICHs & operator[] (NICH *) { return nich; }

    const BBs & operator[] (BB *) const { return bb; }
    const DD16s & operator[] (DD16 *) const { return dd16; }
    const DD256s & operator[] (DD256 *) const { return dd256; }
    const DPDs & operator[] (DPD *) const { return dpd; }
    const GPs & operator[] (GP *) const { return gp; }
    const NICHs & operator[] (NICH *) const { return nich; }
};

} // namespace loom
