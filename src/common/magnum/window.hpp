#pragma once

#ifdef USE_MAGNUM

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace magnum {

class Window : public xr_examples::Window {
    struct Private;
public:
    static void init();

    Window();
    virtual ~Window();
    void create(const glm::uvec2& size) override;
    void makeCurrent() override;
    void doneCurrent() override;
    void setSwapInterval(uint32_t swapInterval);
    void setTitle(const ::std::string& title) override;
    void runWindowLoop(const ::std::function<void()>& handler) override;
    void swapBuffers() override;
    void requestClose() override;
    glm::uvec2 getSize() const override;

private:
    ::std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::magnum

#endif