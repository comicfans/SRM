/*
 * Project: srm-basic Example
 *
 * Author: Eduardo Hopperdietzel
 *
 * Description: This example changes the background color each frame to all
 *              avaliable connectors until CTRL+C is pressed.
 */

#include <SRMCore.h>
#include <SRMDevice.h>
#include <SRMConnector.h>
#include <SRMConnectorMode.h>
#include <SRMListener.h>

#include <SRMList.h>
#include <SRMLog.h>
#include "qr_paint.hpp"
#include <memory>
#include <stdio.h>

#include <GLES2/gl2.h>

#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

float color = 0.f;




struct Timing{
  timespec start_time = {0,0};
  uint64_t total_elapsed_nsec = 0;
  uint64_t counter = 0;
  double next_frame_display_time = 0;
  std::shared_ptr<QrPaint> qr_paint;
  int windowWidth ;
  int windowHeight;

  void update(){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    if(start_time.tv_sec == 0 && start_time.tv_nsec == 0){
      start_time = ts;
      return;
    }

    auto elapsed_nsec = ts.tv_nsec - start_time.tv_nsec;
    const auto minus = elapsed_nsec < 0;
    const auto elapsed_sec = ts.tv_sec - start_time.tv_sec + minus;
    elapsed_nsec = elapsed_nsec + (minus ? 1000000000 : 0);

    total_elapsed_nsec = elapsed_sec * 1000000000 + elapsed_nsec;
    ++counter;

    next_frame_display_time = double(total_elapsed_nsec) * (counter+ 1)/ counter;
  }
};


/* Opens a DRM device */
static int openRestricted(const char *path, int flags, void *userData)
{
    SRM_UNUSED(userData);

    // Here something like libseat could be used instead
    return open(path, flags);
}

/* Closes a DRM device */
static void closeRestricted(int fd, void *userData)
{
    SRM_UNUSED(userData);
    close(fd);
}

static SRMInterface srmInterface =
{
    .openRestricted = &openRestricted,
    .closeRestricted = &closeRestricted
};

static void initializeGL(SRMConnector *connector, void *userData)
{

  Timing *timing = (Timing*)userData;

  timing->windowWidth = srmConnectorModeGetWidth(srmConnectorGetCurrentMode(connector));
  timing->windowHeight = srmConnectorModeGetHeight(srmConnectorGetCurrentMode(connector));

  if(!timing->qr_paint){
    timing->qr_paint = std::make_shared<QrPaint>();
  }

    /* You must not do any drawing here as it won't make it to
     * the screen. */

    SRMConnectorMode *mode = srmConnectorGetCurrentMode(connector);

    glViewport(0, 
               0, 
               srmConnectorModeGetWidth(mode), 
               srmConnectorModeGetHeight(mode));

    // Schedule a repaint (this eventually calls paintGL() later, not directly)
    srmConnectorRepaint(connector);
}

static void paintGL(SRMConnector *connector, void *userData)
{

    glClearColor((sinf(color) + 1.f) / 2.f,
                 (sinf(color * 0.5f) + 1.f) / 2.f,
                 (sinf(color * 0.25f) + 1.f) / 2.f,
                 1.f);

    //color += 0.01f;

    if (color > M_PI*4.f)
        color = 0.f;

    glClear(GL_COLOR_BUFFER_BIT);
    Timing *timing = (Timing*)userData;

    //if(timing->counter > 1650){
      std::string str = std::to_string(uint64_t(timing->next_frame_display_time));

      timing->qr_paint->drawQRCode(str, timing->windowWidth, timing->windowHeight,timing->counter , 2,2);
    //}


    
    srmConnectorRepaint(connector);
}

static void resizeGL(SRMConnector *connector, void *userData)
{
    /* You must not do any drawing here as it won't make it to
     * the screen.
     * This is called when the connector changes its current mode,
     * set with srmConnectorSetMode() */

    // Reuse initializeGL() as it only sets the viewport
    initializeGL(connector, userData);
}

static void pageFlipped(SRMConnector *connector, void *userData)
{
    SRM_UNUSED(connector);


    Timing *timing = (Timing*)userData;


    timing->update();
    


    /* You must not do any drawing here as it won't make it to
     * the screen.
     * This is called when the last rendered frame is now being
     * displayed on screen.
     * Google v-sync for more info. */
}



static void uninitializeGL(SRMConnector *connector, void *userData)
{
    SRM_UNUSED(connector);
    Timing *timing= (Timing*)userData;

    timing->qr_paint.reset();

    /* You must not do any drawing here as it won't make it to
     * the screen.
     * Here you should free any resource created on initializeGL()
     * like shaders, programs, textures, etc. */
}

static SRMConnectorInterface connectorInterface =
{
    .initializeGL = &initializeGL,
    .paintGL = &paintGL,
    .pageFlipped = &pageFlipped,
    .resizeGL = &resizeGL,
    .uninitializeGL = &uninitializeGL
};

static void connectorPluggedEventHandler(SRMListener *listener, SRMConnector *connector)
{
    SRM_UNUSED(listener);

    /* This is called when a new connector is avaliable (E.g. Plugging an HDMI display). */

    /* Got a new connector, let's render on it */
    if (!srmConnectorInitialize(connector, &connectorInterface, NULL))
        SRMError("[srm-basic] Failed to initialize connector %s.",
                 srmConnectorGetModel(connector));
}

static void connectorUnpluggedEventHandler(SRMListener *listener, SRMConnector *connector)
{
    SRM_UNUSED(listener);
    SRM_UNUSED(connector);

    /* This is called when a connector is no longer avaliable (E.g. Unplugging an HDMI display). */

    /* The connnector is automatically uninitialized after this event (if initialized)
     * so calling srmConnectorUninitialize() here is not required. */
}



int main(void)
{

    Timing timing;

    SRMCore *core = srmCoreCreate(&srmInterface, &timing);

    if (!core)
    {
        SRMFatal("[srm-basic] Failed to initialize SRM core.");
        return 1;
    }

    // Subscribe to Udev events
    SRMListener *connectorPluggedEventListener = srmCoreAddConnectorPluggedEventListener(core, &connectorPluggedEventHandler, &timing);
    SRMListener *connectorUnpluggedEventListener = srmCoreAddConnectorUnpluggedEventListener(core, &connectorUnpluggedEventHandler, &timing);

    // Find and initialize avaliable connectors

    // Loop each GPU (device)
    SRMListForeach (deviceIt, srmCoreGetDevices(core))
    {
        SRMDevice *device = (SRMDevice*)srmListItemGetData(deviceIt);

        // Loop each GPU connector (screen)
        SRMListForeach (connectorIt, srmDeviceGetConnectors(device))
        {
            SRMConnector *connector = (SRMConnector*)srmListItemGetData((SRMListItem*)connectorIt);

            if (srmConnectorIsConnected(connector))
            {
                if (!srmConnectorInitialize(connector, &connectorInterface, &timing))
                    SRMError("[srm-basic] Failed to initialize connector %s.",
                             srmConnectorGetModel(connector));
            }
        }
    }

    while (1)
    {
        /* Udev monitor poll DRM devices/connectors hotplugging events (-1 disables timeout).
         * To get a pollable FD use srmCoreGetMonitorFD() */

        if (srmCoreProcessMonitor(core, -1) < 0)
            break;
    }

    /* Unsubscribe to DRM events
     *
     * These listeners are automatically destroyed when calling srmCoreDestroy()
     * so there is no need to free them manually.
     * This is here just to show how to unsubscribe to events on the fly. */

    srmListenerDestroy(connectorPluggedEventListener);
    srmListenerDestroy(connectorUnpluggedEventListener);

    // Finish SRM
    srmCoreDestroy(core);

    return 0;
}
