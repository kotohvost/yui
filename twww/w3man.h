#ifndef _W3MAN_H_
#define _W3MAN_H_

extern "C" int HTLoadMan ( HTRequest * request );

extern "C" HTStream* ManToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream );

extern "C" HTStream* ManIndexToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream );

extern "C" HTStream* AproposToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream );

extern "C" HTStream* InfoToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream );

#endif
