/*
 *  MEM_Fifo.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef MEM_STORE_H
#define MEM_STORE_H

class vsBox2D;
class vsColor;
class vsFog;
class vsLight;
class vsMaterial;
class vsMatrix4x4;
class vsTransform2D;
class vsTransform3D;
class vsVector2D;
class vsVector3D;
class vsVector4D;

class vsStore
{
	char *		m_buffer;
	size_t		m_bufferLength;	// how long is this buffer, really?
	char *		m_bufferEnd;

	char *		m_readHead;
	char *		m_writeHead;

	bool		m_bufferIsExternal;

	void	AssertBytesLeftForWriting(size_t bytes);

	// ordinarily we shouldn't need to do this, but.. for decompressing
	// it becomes important.
	void	ReplaceBuffer( size_t newLength );

public:
			vsStore();
			vsStore( size_t maxSize );
			vsStore( char *buffer, int bufferLength );
			vsStore( const vsStore& store ); // make a copy of the other store
	virtual ~vsStore();

	char *	GetReadHead()	{ return m_readHead; }
	char *	GetWriteHead()	{ return m_writeHead; }
	inline size_t		BytesLeftForWriting() const { return (size_t)(m_bufferEnd - m_writeHead); }
	inline size_t		BytesLeftForReading() const { return (size_t)(m_writeHead - m_readHead); }

	bool	AtEnd()			{ return (m_readHead == m_writeHead); }
	size_t	Length()		{ return (size_t)(m_writeHead - m_buffer); }
	size_t	BufferLength()	{ return m_bufferLength; }
	void	SetLength(size_t l);

	void	Rewind();	// rewind to the start
	void	EraseReadBytes();
	void	AdvanceReadHead(size_t bytes);
	void	AdvanceWriteHead(size_t bytes);
	void	SeekReadHeadTo(size_t pos);
	size_t	GetReadHeadPosition();
	void	RewindWriteHeadTo(size_t pos);
	void	Clear();	// rewind to the start, and erase our contents.

	void	Append( vsStore *o );		// add contents of 'o' to my buffer.

	void	WriteInt8(int8_t value);
	void	WriteUint8(uint8_t value);

	void	WriteInt16(int16_t value);
	void	WriteUint16(uint16_t value);

	void	WriteUint16Native(uint16_t value);	// used for making display lists that will pass these buffers directly to OpenGL;  no converting into global endianness!


	void	WriteInt32(int32_t value);
	void	WriteUint32(uint32_t value);

	void	WriteFloat(float value);

	void	WriteString( const vsString &value );
	void	WriteBuffer( const void *buffer, size_t bufferLength );

	void	WriteVoidStar(void *value);		// WARNING:  DO NOT USE THIS UNLESS YOU REALLY KNOW WHAT YOU'RE DOING!  :)

	int8_t	ReadInt8();
	uint8_t	ReadUint8();
	uint8_t	PeekUint8();

	int16_t	ReadInt16();
	uint16_t	ReadUint16();

	int32_t	ReadInt32();
	uint32_t	ReadUint32();

	float	ReadFloat();

	vsString	ReadString();	// reads out a properly written string (written using 'WriteString()')
	bool		ReadLine( vsString *string );		// reads out a raw string (as per from a network packet or other raw text source, where this vsStore hasn't been constructed by a VectorStorm system)
	void		ReadBufferAsString( vsString *string );
	size_t		ReadBuffer( void *buffer, size_t bufferLength );

	void *		ReadVoidStar();					// WARNING:  DO NOT USE THIS UNLESS YOU REALLY KNOW WHAT YOU'RE DOING!  :)

	void		WriteVector2D(const vsVector2D &v);
	void		ReadVector2D(vsVector2D *v);

	void		WriteVector3D(const vsVector3D &v);
	void		ReadVector3D(vsVector3D *v);

	void		WriteVector4D(const vsVector4D &v);
	void		ReadVector4D(vsVector4D *v);

	void		WriteColor(const vsColor &c);
	void		ReadColor(vsColor *c);

	void		WriteLight(const vsLight &l);
	void		ReadLight(vsLight *l);

	void		WriteFog(const vsFog &f);
	void		ReadFog(vsFog *f);

	void		WriteTransform2D(const vsTransform2D &t);
	void		ReadTransform2D(vsTransform2D *t);

	void		WriteMatrix4x4(const vsMatrix4x4 &m);
	void		ReadMatrix4x4(vsMatrix4x4 *m);

	void		WriteMaterial(const vsMaterial &m);
	void		ReadMaterial(vsMaterial *m);

	void		WriteBox2D(const vsBox2D &box);
	void		ReadBox2D(vsBox2D *box);

	bool		Compress(); // gzip the store, reset read head to start.  Returns true on success.
	bool		Expand(); // ungzip the store, reset read head to start and write head to end.  Returns true on success.
};

#endif // MEM_FIFO_H

