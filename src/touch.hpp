#pragma once
// Deterministic touch gesture engine. OS-independent C++20; no UI/runtime dependencies.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace touch {
struct Contact { std::int64_t id; float x, y; bool pen=false; };
enum class Type { Move, Down, Up, Cancel, Scroll, Zoom, LongPress };
struct Event { Type type; float x=0, y=0; float value=0, second=0; bool pen=false; };
using Events = std::vector<Event>;
class Controller {
    enum class Mode { Idle, Pointer, Scroll, Multi, Suppressed } mode_=Mode::Idle;
    Contact first_{};
    float x0_=0,y0_=0,cx_=0,cy_=0,separation_=0;
    float velocity_x_=0,velocity_y_=0;
    double down_at_=0,last_at_=0,last_tick_at_=-1;
    bool holding_=false, long_pressed_=false, momentum_=false;
    static float distance(Contact a,Contact b) {return std::hypot(a.x-b.x,a.y-b.y);}
    static float capped(float v,float cap) {return std::clamp(v,-cap,cap);}
public:
    bool active() const {return mode_!=Mode::Idle;}
    void reset() { *this=Controller{}; }
    // Pass the complete currently-active pointer set after each OS touch event.
    // allow_scroll must be false when the user is manipulating an active widget (e.g. slider).
    Events update(const std::vector<Contact>& contacts,double now,bool allow_scroll=true) {
        Events out;
        now=std::max(now,last_at_);
        const float dt=(float)std::max(.001,now-last_at_);
        const double idleBeforeEvent=now-last_at_;
        last_at_=now;
        auto send=[&](Type t,float x,float y,float value=0,float second=0,bool pen=false){out.push_back({t,x,y,value,second,pen});};
        if(contacts.empty()) {
            if(mode_==Mode::Pointer)send(Type::Up,first_.x,first_.y,0,0,first_.pen);
            if(mode_==Mode::Scroll || mode_==Mode::Multi)
                momentum_=std::hypot(velocity_x_,velocity_y_)>.6f;
            if(mode_==Mode::Scroll && idleBeforeEvent>.1)momentum_=false;
            mode_=Mode::Idle;holding_=false;
            return out;
        }
        if(contacts.size()>1) {
            const Contact& a=contacts[0],&b=contacts[1];
            const float mx=(a.x+b.x)*.5f,my=(a.y+b.y)*.5f;
            const float length=std::max(1.f,distance(a,b));
            if(mode_!=Mode::Multi) {
                if(mode_==Mode::Pointer)send(Type::Cancel,first_.x,first_.y,0,0,first_.pen);
                mode_=Mode::Multi; cx_=mx;cy_=my;separation_=length;
                velocity_x_=velocity_y_=0; momentum_=false;
            } else {
                float dx=mx-cx_,dy=my-cy_;
                if(std::abs(dx)>0.01f||std::abs(dy)>0.01f){
                    // ImGui mouse wheel units are not pixels; ~60 screen points per wheel unit.
                    send(Type::Scroll,cx_,cy_,-dx/60.f,-dy/60.f);
                    velocity_x_=capped(-dx/dt/60.f,20.f);
                    velocity_y_=capped(-dy/dt/60.f,20.f);
                }
                const float scale=length/separation_;
                if(scale<.98f||scale>1.02f) send(Type::Zoom,mx,my,scale);
                cx_=mx;cy_=my;separation_=length;
            }
            return out;
        }
        const Contact p=contacts.front();
        if(mode_==Mode::Multi||mode_==Mode::Suppressed) {
            // Don't click when a second finger leaves. Keep kinetic velocity and
            // start coasting only after the remaining finger also leaves.
            if(mode_==Mode::Multi)
                momentum_=std::hypot(velocity_x_,velocity_y_)>.6f;
            mode_=Mode::Suppressed;
            return out;
        }
        if(mode_==Mode::Idle) {
            momentum_=false;mode_=Mode::Pointer;first_=p;
            x0_=p.x;y0_=p.y;down_at_=now;holding_=true;long_pressed_=false;
            send(Type::Move,p.x,p.y,0,0,p.pen);
            send(Type::Down,p.x,p.y,0,0,p.pen);
            return out;
        }
        if(mode_==Mode::Pointer) {
            const float displacement=std::hypot(p.x-x0_,p.y-y0_);
            // Drag sliders/buttons as normal; pan non-interactive content with one finger.
            if(allow_scroll && displacement>13.f && std::abs(p.y-y0_)>std::abs(p.x-x0_)*1.15f) {
                send(Type::Cancel,first_.x,first_.y,0,0,first_.pen);
                mode_=Mode::Scroll;holding_=false;
                velocity_x_=velocity_y_=0;
                // Transfer the complete gesture delta, including the initial
                // touch slop, to scrolling. Don't lose the first 13 points.
                first_.x=x0_;first_.y=y0_;
            } else {
                first_=p;
                send(Type::Move,p.x,p.y,0,0,p.pen);
                return out;
            }
        }
        if(mode_==Mode::Scroll) {
            const float dy=p.y-first_.y,dx=p.x-first_.x;
            send(Type::Scroll,p.x,p.y,-dx/60.f,-dy/60.f);
            // Velocity filtering applies ONLY to finger flicks, never to
            // native trackpad/macOS momentum events, which are already shaped.
            const float vx=capped(-dx/dt/60.f,90.f);
            const float vy=capped(-dy/dt/60.f,90.f);
            velocity_x_=velocity_x_*.35f+vx*.65f;
            velocity_y_=velocity_y_*.35f+vy*.65f;
            first_=p;
        }
        return out;
    }
    Events tick(double now) {
        Events out;
        // Frame ticks and pointer events MUST have independent clocks:
        // mixing them silently biases gesture velocity and makes inertia
        // dependent on display refresh rate.
        double dt=last_tick_at_<0 ? 0 : std::clamp(now-last_tick_at_,0.,.050);
        last_tick_at_=now;
        if(mode_==Mode::Pointer&&holding_&&!long_pressed_&&
           now-down_at_>=.55&&std::hypot(first_.x-x0_,first_.y-y0_)<=10.f) {
            // Long press opens ImGui's right-button context menu.
            out.push_back({Type::Up,first_.x,first_.y,0,0,first_.pen});
            out.push_back({Type::LongPress,first_.x,first_.y,0,0,first_.pen});
            holding_=false;long_pressed_=true;mode_=Mode::Suppressed;
        }
        if(mode_==Mode::Idle&&momentum_&&dt>0) {
            constexpr float frictionRate=4.6f;
            const float drag=std::exp(-frictionRate*(float)dt);
            const float integrated=(1.f-drag)/frictionRate;
            // Analytically integrated exponential decay is frame-rate
            // invariant (60/120 Hz produce the same travelled distance).
            out.push_back({Type::Scroll,first_.x,first_.y,
                           velocity_x_*integrated,velocity_y_*integrated});
            velocity_x_*=drag;velocity_y_*=drag;
            if(std::hypot(velocity_x_,velocity_y_)<.065f)momentum_=false;
        }
        return out;
    }
};
}
