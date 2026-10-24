// SPDX-License-Identifier: GPL-3.0-only
// Read-only active CoreMIDI inventory. No client, port, connection or send API.
// Build against the actual macOS SDK; Python never guesses the CoreMIDI ABI.
#import <Foundation/Foundation.h>
#import <CoreMIDI/CoreMIDI.h>
#include <stdint.h>
#include <stdio.h>

static id numberProperty(MIDIObjectRef object, CFStringRef key)
{
    SInt32 value = 0;
    return MIDIObjectGetIntegerProperty(object, key, &value) == noErr ? @(value) : [NSNull null];
}

static NSDictionary *properties(MIDIObjectRef object)
{
    CFPropertyListRef raw = NULL;
    OSStatus status = MIDIObjectGetProperties(object, &raw, false);
    if (status != noErr) {
        if (raw) CFRelease(raw);
        return nil;
    }
    id value = CFBridgingRelease(raw);
    return [value isKindOfClass:[NSDictionary class]] ? value : nil;
}

static id usbNumber(NSDictionary *props, NSString *key)
{
    id value = props[key];
    if (![value isKindOfClass:[NSNumber class]] || CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID())
        return [NSNull null];
    double number = [value doubleValue];
    long long integer = [value longLongValue];
    if (integer <= 0 || integer > UINT32_MAX || number != (double)integer)
        return [NSNull null];
    return @(integer);
}

static NSDictionary *endpointRecord(MIDIEndpointRef endpoint)
{
    MIDIEntityRef entity = 0;
    MIDIDeviceRef device = 0;
    if (!endpoint || MIDIEndpointGetEntity(endpoint, &entity) != noErr || !entity ||
        MIDIEntityGetDevice(entity, &device) != noErr || !device)
        return nil; // includes virtual endpoints lacking a hardware parent
    NSDictionary *props = properties(device);
    if (!props) return nil;
    id serial = props[@"SerialNumber"];
    // Never serialize a serial value, device name, uniqueID or cached device list.
    BOOL serialPresent = [serial isKindOfClass:[NSString class]] && [serial length] > 0;
    return @{
        @"endpoint": @(endpoint), @"entity": @(entity), @"device": @(device),
        @"device_offline": numberProperty(device, kMIDIPropertyOffline),
        @"entity_offline": numberProperty(entity, kMIDIPropertyOffline),
        @"endpoint_offline": numberProperty(endpoint, kMIDIPropertyOffline),
        @"usb_vendor_product": usbNumber(props, @"USBVendorProduct"),
        @"usb_location": usbNumber(props, @"USBLocationID"),
        @"serial_property_present": @(serialPresent)
    };
}

static NSArray *activeEndpoints(BOOL sources, NSUInteger *skipped)
{
    ItemCount count = sources ? MIDIGetNumberOfSources() : MIDIGetNumberOfDestinations();
    if (count > 512) return nil;
    NSMutableArray *rows = [NSMutableArray array];
    for (ItemCount index = 0; index < count; index++) {
        MIDIEndpointRef endpoint = sources ? MIDIGetSource(index) : MIDIGetDestination(index);
        NSDictionary *row = endpointRecord(endpoint);
        if (row) [rows addObject:row]; else (*skipped)++;
    }
    return rows;
}

int main(int argc, const char *argv[])
{
    (void)argv;
    @autoreleasepool {
        if (argc != 1) return 2;
        NSUInteger skippedSources = 0, skippedDestinations = 0;
        NSArray *sources = activeEndpoints(YES, &skippedSources);
        NSArray *destinations = activeEndpoints(NO, &skippedDestinations);
        if (!sources || !destinations) return 2;
        NSDictionary *snapshot = @{
            @"schema": @"x0x37-coremidi-active-inventory-v1",
            @"sources": sources, @"destinations": destinations,
            @"skipped_sources": @(skippedSources), @"skipped_destinations": @(skippedDestinations)
        };
        NSData *json = [NSJSONSerialization dataWithJSONObject:snapshot options:0 error:NULL];
        if (!json || fwrite(json.bytes, 1, json.length, stdout) != json.length) return 2;
        return 0;
    }
}
