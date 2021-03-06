#pragma once

#include "model_element.h"
#include "util.h"

#include <adept_arrays.h>
#include <boost/optional.hpp>
#include <set>

class HingeModel : public ModelElement
{
  public:
    class Segment;

    class HingeCollisionZone : public ModelElement
    {
      private:
        void SetupEquationsThis() override;
        void ComputeScoreThis(adept::aReal &score) const override;
        void ApplyGradientThis(double score_normalization) override;
        void VisualiseThis(std::vector<Visualisation::Object> &objects) const override;

        Vector<2, false> position_;
        float collision_zone_side_;

        std::set<Segment *> segments_;

      public:
        HingeCollisionZone(HingeModel *model, Vector<2, false> position_, float side);
        bool Collides(Segment *seg) const;
        void RegisterSegment(HingeModel::Segment *segment_);
    };

    class Segment : public ModelElement, public Visualisation::TooltipInterface
    {
        void LinkBackward(Segment *previous);
        void VisualiseThis(std::vector<Visualisation::Object> &objects) const override;

      protected:
        Segment *next_;
        Segment *previous_;

        HingeModel *model_;
        const bool solid_;
        SDL2pp::Color vis_color_;

        Segment(HingeModel *model, bool solid, SDL2pp::Color vis_color);

      public:
        virtual ~Segment() = default;
        virtual void LinkForward(Segment *next);
        static bool Intersects(const Segment *s1, const Segment *s2);
        virtual Vector<2, false> GetPosition() const = 0;
    };

    class Hinge : public Segment
    {
      private:
        void SetupEquationsThis() override;
        void ComputeScoreThis(adept::aReal &score) const override;
        void ApplyGradientThis(double score_normalization) override;

        std::string GetTooltip() const override;

        static SDL2pp::Color SpeedToColor(double speed);

        const Vector<2, false> zero_position_;
        mutable Vector<2, true> position_;
        adept::aReal crossposition_;
        Vector<2, false> crossposition_vector_;

        adept::aReal speed_;
        mutable double last_centrifugal_force_;
        const double width_;
        const double forward_;

      public:
        Hinge(HingeModel *model, Vector<2, false> position, double width, double forward);

        void LinkForward(Segment *next) override;
        Vector<2, false> GetPosition() const override;
        double GetCrossposition() const;
        double GetSpeed() const;
        Vector<2, false> GetCrosspositionVector() const;
        double GetForward() const;
        Hinge *GetNext() const;
        Hinge *GetPrevious() const;
        void SetCrossposition(double new_cp);
        void SetSpeed(double new_cp);
    };

    class BandSegement : public Segment
    {
      private:
        void SetupEquationsThis() override;
        void ComputeScoreThis(adept::aReal &score) const override;
        void ApplyGradientThis(double score_normalization) override;

        std::string GetTooltip() const override;

        Vector<2, false> position_;

      public:
        BandSegement(HingeModel *model, Vector<2, false> position);
        Vector<2, false> GetPosition() const override;
    };

  private:
    std::vector<std::vector<HingeCollisionZone *>> collision_zones_;
    std::pair<int, int> CoordinatesToCollisionZone(Vector<2, false> pos);

    uint32_t width_;
    uint32_t height_;
    double collision_zone_side_;
    double alpha_;
    double max_centrifugal_force_;
    double max_acceleration_;
    boost::optional<std::pair<double, double>> first_last_score_;

    Hinge *first_hinge_;

    void SetupEquationsThis() override;
    void ComputeScoreThis(adept::aReal &score) const override;
    void ApplyGradientThis(double score_normalization) override;
    void VisualiseThis(std::vector<Visualisation::Object> &objects) const override;

    void AddHinge(Hinge *h);
    void AddBandSegment(BandSegement *bs);

    Log log_{"HingeModel"};

  public:
    HingeModel(uint32_t width, uint32_t height, float collision_zone_side);
    Hinge *GetFirstHinge() const;

    double Optimize(adept::Stack &stack);
};