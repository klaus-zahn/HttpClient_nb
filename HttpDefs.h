/**
 * ****************************************************************************
 * 
 *  @brief 		Definition of common HTTP definitions. Refer to RFCs for this and 
 *				further definitions.
 *				
 *
 *				Project: Smart Person Counter
 * 						
 * ---------------------------------------------------------------------------
 *
 *  @author		Roman Sidler
 *					
 *  @copyright	CC Innovation in Intelligent Multimedia Sensor
 *              Networks at Lucerne University of Applied Sciences
 * 				and Arts T&A, Switzerland.				
 *
 ****************************************************************************
 */

#ifndef HTTPDEFS_H
#define HTTPDEFS_H

class HttpDefs
{
public:
    static const char *MIME_TEXT_PLAIN;
	static const char *MIME_TEXT_HTML;
    static const char *MIME_TEXT_XML;
    static const char *MIME_IMAGE_JPEG;
    static const char *MIME_IMAGE_BMP;
	static const char *MIME_IMAGE_TOF;
    static const char *MIME_APPLICATION_OCTETSTREAM;
    static const char *MIME_MULTIPART_FORMDATA;
    static const char *MIME_MULTIPART_MIXEDREPLACE;
};
#endif /*HTTPDEFS_H*/
