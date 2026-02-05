/*
Copyright (C) 2008 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// r_framebuffer.c - Framebuffer Objects support

#include "r_local.h"

#define MAX_FRAMEBUFFER_OBJECTS 1024

typedef struct {
	int registrationSequence; // -1 if builtin
	unsigned int objectID;
	unsigned int depthRenderBuffer;
	unsigned int stencilRenderBuffer;
	int width, height;
	image_t *textures[FBO_TEXTURE_COUNT];
} r_fbo_t;

static bool r_frambuffer_objects_initialized;
static int r_bound_framebuffer_objectID;
static r_fbo_t *r_bound_framebuffer_object;
static int r_num_framebuffer_objects;
static r_fbo_t r_framebuffer_objects[MAX_FRAMEBUFFER_OBJECTS];

/*
 * RFB_Init
 */
void RFB_Init( void )
{
	r_num_framebuffer_objects = 0;
	memset( r_framebuffer_objects, 0, sizeof( r_framebuffer_objects ) );

	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	r_bound_framebuffer_objectID = 0;
	r_bound_framebuffer_object = NULL;

	r_frambuffer_objects_initialized = true;
}

/*
 * RFB_DeleteObject
 *
 * Delete framebuffer object along with attached render buffer
 */
static void RFB_DeleteObject( r_fbo_t *fbo )
{
	if( fbo->depthRenderBuffer ) {
		qglDeleteRenderbuffers( 1, &( fbo->depthRenderBuffer ) );
		fbo->depthRenderBuffer = 0;
	}

	if( fbo->stencilRenderBuffer ) {
		qglDeleteRenderbuffers( 1, &( fbo->stencilRenderBuffer ) );
		fbo->stencilRenderBuffer = 0;
	}

	if( fbo->objectID ) {
		qglDeleteFramebuffers( 1, &( fbo->objectID ) );
		fbo->objectID = 0;
	}
}

/*
 * RFB_RegisterObject
 */
int RFB_RegisterObject( int width, int height, bool builtin, bool depthRB, bool stencilRB )
{
	int i;
	GLuint fbID;
	GLuint rbID;
	r_fbo_t *fbo;

	if( !r_frambuffer_objects_initialized )
		return 0;

	for( i = 0, fbo = r_framebuffer_objects; i < r_num_framebuffer_objects; i++, fbo++ ) {
		if( !fbo->objectID ) {
			// free slot
			goto found;
		}
	}

	if( i == MAX_FRAMEBUFFER_OBJECTS ) {
		Com_Printf( S_COLOR_YELLOW "RFB_RegisterObject: framebuffer objects limit exceeded\n" );
		return 0;
	}

	i = r_num_framebuffer_objects++;
	fbo = r_framebuffer_objects + i;

found:
	qglGenFramebuffers( 1, &fbID );
	memset( fbo, 0, sizeof( *fbo ) );
	fbo->objectID = fbID;
	if( builtin )
		fbo->registrationSequence = -1;
	else
		fbo->registrationSequence = rsh.registrationSequence;
	fbo->width = width;
	fbo->height = height;

	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo->objectID );

	// until a color texture is attached, don't enable drawing to the buffer
	qglDrawBuffer( GL_NONE );
	qglReadBuffer( GL_NONE );

	if( depthRB ) {
		int format;

		qglGenRenderbuffers( 1, &rbID );
		fbo->depthRenderBuffer = rbID;
		qglBindRenderbuffer( GL_RENDERBUFFER, rbID );

		if( stencilRB )
			format = GL_DEPTH24_STENCIL8_EXT;
		else if( glConfig.ext.depth24 )
			format = GL_DEPTH_COMPONENT24;
		else if( glConfig.ext.depth_nonlinear )
			format = GL_DEPTH_COMPONENT16_NONLINEAR_NV;
		else
			format = GL_DEPTH_COMPONENT16;
		qglRenderbufferStorage( GL_RENDERBUFFER, format, width, height );

		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbID );
		if( stencilRB )
			qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbID );

		qglBindRenderbuffer( GL_RENDERBUFFER, 0 );
	}

	if( r_bound_framebuffer_objectID )
		qglBindFramebuffer( GL_FRAMEBUFFER, r_bound_framebuffer_object->objectID );
	else
		qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	return i + 1;
}

/*
 * RFB_UnregisterObject
 */
void RFB_UnregisterObject( int object )
{
	r_fbo_t *fbo;

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( !object ) {
		return;
	}

	fbo = r_framebuffer_objects + object - 1;
	RFB_DeleteObject( fbo );
}

/*
 * RFB_TouchObject
 */
void RFB_TouchObject( int object )
{
	r_fbo_t *fbo;

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( !object ) {
		return;
	}

	fbo = r_framebuffer_objects + object - 1;
	fbo->registrationSequence = rsh.registrationSequence;
}

/*
 * RFB_BoundObject
 */
int RFB_BoundObject( void )
{
	return r_bound_framebuffer_objectID;
}

/*
 * RFB_BindObject
 *
 * DO NOT call this function directly, use R_BindFrameBufferObject instead.
 */
void RFB_BindObject( int object )
{
	if( !object ) {
		if( r_frambuffer_objects_initialized ) {
			qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
		}
		r_bound_framebuffer_objectID = 0;
		r_bound_framebuffer_object = NULL;
		return;
	}

	if( !r_frambuffer_objects_initialized ) {
		return;
	}

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( object <= 0 || object > r_num_framebuffer_objects ) {
		return;
	}

	if( r_bound_framebuffer_objectID == object ) {
		return;
	}

	r_bound_framebuffer_objectID = object;
	r_bound_framebuffer_object = r_framebuffer_objects + object - 1;
	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, r_bound_framebuffer_object->objectID );
}

/*
 * RFB_SetupMRT
 */
void RFB_SetupMRT( int object )
{
	if( !object ) {
		return;
	}

	int numColorAttachments = 0;
	GLenum drawBuffers[FBO_TEXTURE_COUNT - 1]; // Exclude depth texture

	for( FBO_TEXTURE_TYPE i = FBO_TEXTURE_COLOR; i < FBO_TEXTURE_DEPTH; i++ ) {
		if( r_bound_framebuffer_object->textures[i] ) {
			drawBuffers[numColorAttachments++] = GL_COLOR_ATTACHMENT0_EXT + i;
		}
	}

	if( numColorAttachments > 0 )
		qglDrawBuffers( numColorAttachments, drawBuffers );
}

/*
 * RFB_AttachTextureToObject
 */
void RFB_AttachTextureToObject( int object, image_t *texture, FBO_TEXTURE_TYPE type )
{
	r_fbo_t *fbo;
	int attachment;

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( object <= 0 || object > r_num_framebuffer_objects ) {
		return;
	}

	assert( texture != NULL );
	if( !texture ) {
		return;
	}

	assert( type < FBO_TEXTURE_COUNT );

	fbo = r_framebuffer_objects + object - 1;
	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo->objectID );

	if( texture->flags & IT_DEPTH ) {
		assert( type == FBO_TEXTURE_DEPTH );
		attachment = GL_DEPTH_ATTACHMENT_EXT;
		fbo->textures[FBO_TEXTURE_DEPTH] = texture;
	} else {
		attachment = GL_COLOR_ATTACHMENT0_EXT + type;
		fbo->textures[type] = texture;
	}

	texture->fbo = object;

	// attach texture
	qglFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->texnum, 0 );
	if( ( texture->flags & ( IT_DEPTH | IT_STENCIL ) ) == ( IT_DEPTH | IT_STENCIL ) ) {
		qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->texnum, 0 );
	}

	qglBindFramebuffer( GL_FRAMEBUFFER, r_bound_framebuffer_objectID ? r_bound_framebuffer_object->objectID : 0 );
}

/*
 * RFB_GetObjectTextureAttachment
 */
image_t *RFB_GetObjectTextureAttachment( int object, FBO_TEXTURE_TYPE type )
{
	r_fbo_t *fbo;

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( object <= 0 || object > r_num_framebuffer_objects ) {
		return NULL;
	}

	fbo = r_framebuffer_objects + object - 1;
	return fbo->textures[type];
}

/*
 * RFB_BlitObject
 *
 * The target FBO must be equal or greater in both dimentions than
 * the currently bound FBO!
 */
void RFB_BlitObject( int dest, int bitMask, int mode )
{
	int bits;
	int dx, dy, dw, dh;
	r_fbo_t *fbo = r_bound_framebuffer_object, *destfbo = r_framebuffer_objects + dest - 1;

	if( !r_bound_framebuffer_object ) {
		return;
	}
	if( !glConfig.ext.framebuffer_blit ) {
		return;
	}

	assert( dest > 0 && dest <= r_num_framebuffer_objects );
	if( dest <= 0 || dest > r_num_framebuffer_objects ) {
		return;
	}

	bits = bitMask;
	if( !bits ) {
		return;
	}

	RB_ApplyScissor();

	switch( mode ) {
		case FBO_COPY_CENTREPOS:
			dx = ( destfbo->width - fbo->width ) / 2;
			dy = ( destfbo->height - fbo->height ) / 2;
			dw = fbo->width;
			dh = fbo->height;
			break;
		case FBO_COPY_INVERT_Y:
			dx = 0;
			dy = destfbo->height - fbo->height;
			dw = fbo->width;
			dh = fbo->height;
			break;
		default:
			dx = 0;
			dy = 0;
			dw = fbo->width;
			dh = fbo->height;
			break;
	}

	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	qglBindFramebuffer( GL_READ_FRAMEBUFFER, fbo->objectID );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, destfbo->objectID );
	qglBlitFramebuffer( 0, 0, fbo->width, fbo->height, dx, dy, dx + dw, dy + dh, bits, GL_NEAREST );
	qglBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
	qglBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fbo->objectID );

	assert( qglGetError() == GL_NO_ERROR );
}

/*
 * RFB_CheckObjectStatus
 *
 * Boolean, returns false in case of error
 */
bool RFB_CheckObjectStatus( void )
{
	GLenum status;

	if( !r_frambuffer_objects_initialized )
		return false;

	status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
	switch( status ) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return true;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			return false;
		default:
			// programming error; will fail on all hardware
			assert( 0 );
	}

	return false;
}

/*
 * RFB_GetObjectSize
 */
void RFB_GetObjectSize( int object, int *width, int *height )
{
	r_fbo_t *fbo;

	if( !object ) {
		*width = glConfig.width;
		*height = glConfig.height;
		return;
	}

	assert( object > 0 && object <= r_num_framebuffer_objects );
	if( object <= 0 || object > r_num_framebuffer_objects ) {
		return;
	}

	fbo = r_framebuffer_objects + object - 1;
	*width = fbo->width;
	*height = fbo->height;
}

/*
 * RFB_FreeUnusedObjects
 */
void RFB_FreeUnusedObjects( void )
{
	int i;
	r_fbo_t *fbo = r_framebuffer_objects;
	int registrationSequence;

	if( !r_frambuffer_objects_initialized )
		return;

	for( i = 0; i < r_num_framebuffer_objects; i++, fbo++ ) {
		registrationSequence = fbo->registrationSequence;
		if( ( registrationSequence < 0 ) || ( registrationSequence == rsh.registrationSequence ) ) {
			continue;
		}
		RFB_DeleteObject( fbo );
	}
}

/*
 * RFB_Shutdown
 *
 * Delete all registered framebuffer and render buffer objects, clear memory
 */
void RFB_Shutdown( void )
{
	int i;

	if( !r_frambuffer_objects_initialized )
		return;

	for( i = 0; i < r_num_framebuffer_objects; i++ ) {
		RFB_DeleteObject( r_framebuffer_objects + i );
	}

	qglBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	r_bound_framebuffer_objectID = 0;

	r_frambuffer_objects_initialized = false;
	r_num_framebuffer_objects = 0;
	memset( r_framebuffer_objects, 0, sizeof( r_framebuffer_objects ) );
}
