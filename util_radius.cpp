/*
 * Copyright (c) 2010, JANET(UK)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of JANET(UK) nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gssapiP_eap.h"

VALUE_PAIR *
gss_eap_radius_attr_provider::copyAvps(const VALUE_PAIR *in)
{
    return NULL;
}

gss_eap_radius_attr_provider::gss_eap_radius_attr_provider(void)
{
    m_avps = NULL;
    m_authenticated = false;
}

gss_eap_radius_attr_provider::~gss_eap_radius_attr_provider(void)
{
    if (m_avps != NULL)
        rc_avpair_free(m_avps);
}

bool
gss_eap_radius_attr_provider::initFromExistingContext(const gss_eap_attr_ctx *manager,
                                                      const gss_eap_attr_provider *ctx)
{
    if (!gss_eap_attr_provider::initFromExistingContext(manager, ctx))
        return false;

    return true;
}

bool
gss_eap_radius_attr_provider::initFromGssContext(const gss_eap_attr_ctx *manager,
                                                 const gss_cred_id_t gssCred,
                                                 const gss_ctx_id_t gssCtx)
{
    if (!gss_eap_attr_provider::initFromGssContext(manager, gssCred, gssCtx))
        return false;

    return true;
}

bool
gss_eap_radius_attr_provider::getAttributeTypes(gss_eap_attr_enumeration_cb addAttribute, void *data) const
{
    return true;
}

void
gss_eap_radius_attr_provider::setAttribute(int complete,
                                           const gss_buffer_t attr,
                                           const gss_buffer_t value)
{
}

void
gss_eap_radius_attr_provider::deleteAttribute(const gss_buffer_t value)
{
}

bool
gss_eap_radius_attr_provider::getAttribute(const gss_buffer_t attr,
                                           int *authenticated,
                                           int *complete,
                                           gss_buffer_t value,
                                           gss_buffer_t display_value,
                                           int *more) const
{
    return false;
}

bool
gss_eap_radius_attr_provider::getAttribute(unsigned int attr,
                                           int *authenticated,
                                           int *complete,
                                           gss_buffer_t value,
                                           gss_buffer_t display_value,
                                           int *more) const
{
    return false;
}

gss_any_t
gss_eap_radius_attr_provider::mapToAny(int authenticated,
                                       gss_buffer_t type_id) const
{
    if (authenticated && !m_authenticated)
        return (gss_any_t)NULL;

    return (gss_any_t)copyAvps(m_avps);
}

void
gss_eap_radius_attr_provider::releaseAnyNameMapping(gss_buffer_t type_id,
                                                    gss_any_t input) const
{
    rc_avpair_free((VALUE_PAIR *)input);
}

void
gss_eap_radius_attr_provider::exportToBuffer(gss_buffer_t buffer) const
{
    buffer->length = 0;
    buffer->value = NULL;
}

bool
gss_eap_radius_attr_provider::initFromBuffer(const gss_eap_attr_ctx *ctx,
                                             const gss_buffer_t buffer)
{
    if (!gss_eap_attr_provider::initFromBuffer(ctx, buffer))
        return false;

    return true;
}

bool
gss_eap_radius_attr_provider::init(void)
{
    gss_eap_attr_ctx::registerProvider(ATTR_TYPE_RADIUS,
                                       "urn:ietf:params:gss-eap:radius-avp",
                                       gss_eap_radius_attr_provider::createAttrContext);
    return true;
}

void
gss_eap_radius_attr_provider::finalize(void)
{
    gss_eap_attr_ctx::unregisterProvider(ATTR_TYPE_RADIUS);
}

gss_eap_attr_provider *
gss_eap_radius_attr_provider::createAttrContext(void)
{
    return new gss_eap_radius_attr_provider;
}

OM_uint32
addAvpFromBuffer(OM_uint32 *minor,
                 rc_handle *rh,
                 VALUE_PAIR **vp,
                 int type,
                 gss_buffer_t buffer)
{
    if (rc_avpair_add(rh, vp, type, buffer->value, buffer->length, 0) == NULL) {
        return GSS_S_FAILURE;
    }

    return GSS_S_COMPLETE;
}

OM_uint32
getBufferFromAvps(OM_uint32 *minor,
                  VALUE_PAIR *vps,
                  int type,
                  gss_buffer_t buffer,
                  int concat)
{
    VALUE_PAIR *vp;
    unsigned char *p;

    buffer->length = 0;
    buffer->value = NULL;

    vp = rc_avpair_get(vps, type, 0);
    if (vp == NULL)
        return GSS_S_UNAVAILABLE;

    do {
        buffer->length += vp->lvalue;
    } while (concat && (vp = rc_avpair_get(vp->next, type, 0)) != NULL);

    buffer->value = GSSEAP_MALLOC(buffer->length);
    if (buffer->value == NULL) {
        *minor = ENOMEM;
        return GSS_S_FAILURE;
    }

    p = (unsigned char *)buffer->value;

    for (vp = rc_avpair_get(vps, type, 0);
         concat && vp != NULL;
         vp = rc_avpair_get(vp->next, type, 0)) {
        memcpy(p, vp->strvalue, vp->lvalue);
        p += vp->lvalue;
    }

    *minor = 0;
    return GSS_S_COMPLETE;
}

OM_uint32
gssEapRadiusAttrProviderInit(OM_uint32 *minor)
{
    return gss_eap_radius_attr_provider::init()
        ? GSS_S_COMPLETE : GSS_S_FAILURE;
}

OM_uint32
gssEapRadiusAttrProviderFinalize(OM_uint32 *minor)
{
    gss_eap_radius_attr_provider::finalize();
    return GSS_S_COMPLETE;
}
