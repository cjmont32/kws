#include "cgi.h"
#include "util.h"
#include "html.h"

void cgi_main()
{
    html_printf("<input id=\"should_use_extensions\" type=\"hidden\" value=\"%d\" />", !cgi_server_is_node());
    html_printf("<div class=\"container\">");
    html_printf("<h2>Keyword Search Web App Submission</h2>");
    html_printf("<h3>Created by Cory Montgomery</h3>");
    html_printf("<text>Example questions:</text>");
    html_printf("<br/>");
    html_printf("<br/>");
    html_printf("<div>%s</div>", "What is the diameter of the moon?");
    html_printf("<div>%s</div>", "What is the temperature of the Sun?");
    html_printf("<div>%s</div>", "What is the mass of an electron?");
    html_printf("<div>%s</div>", "How fast is plaid speed?");
    html_printf("<br/>");
    html_printf("<br/>");
    html_printf("<input type=\"text\" id=\"kws\" name=\"kws\" placeholder=\"enter search query\" />");
    html_printf("<div id=\"results\"></div>");
    html_printf("<br/>");

    html_printf(
        "Note: Results are ranked by how many keywords matched. The 1st result should be the best available match.");

    html_printf("</div>");
}