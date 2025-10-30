//go:build windows && cgo

package soundlibwrap

/*
#include <stdint.h>
#include <stdbool.h>
#include "SoundAgentApi.h"
*/
import "C"

//export cgoSaaDefaultRenderChanged
func cgoSaaDefaultRenderChanged(present C.int) {
	NotifyDefaultRenderChanged(present != 0)
}

//export cgoSaaDefaultCaptureChanged
func cgoSaaDefaultCaptureChanged(present C.int) {
	NotifyDefaultCaptureChanged(present != 0)
}

//export cgoSaaGotLogMessage
func cgoSaaGotLogMessage(msg C.SaaLogMessage) {
	timestamp := C.GoString(&msg.Timestamp[0])
	level := C.GoString(&msg.Level[0])
	content := C.GoString(&msg.Content[0])
	NotifyGotLogMessage(timestamp, level, content)
}
