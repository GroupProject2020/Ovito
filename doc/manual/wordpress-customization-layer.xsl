<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:import href="docbook-xsl/html/chunk.xsl"/>

  <xsl:output method="html" encoding="ISO-8859-1" indent="yes" />

  <xsl:param name="xref.with.number.and.title" select="'0'"/>
  <xsl:param name="callout.graphics" select="'0'"/>
  <xsl:param name="callout.unicode" select="'1'"/>

  <xsl:param name="wordpress.dir">..</xsl:param>
  <xsl:param name="use.extensions" select="0"/>

  <xsl:param name="chunk.section.depth" select="3"/>
  <xsl:param name="chunk.first.sections" select="1"/>
  <xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
  <xsl:param name="use.id.as.filename" select="1"/>
  <xsl:param name="html.stylesheet" select="'manual-wordpress.css'"/>
  <xsl:param name="chapter.autolabel" select="0"/>
  <xsl:param name="section.autolabel" select="0"/>
  <xsl:param name="section.label.includes.component.label" select="1"/>
  <xsl:param name="xref.with.number.and.title" select="0"/>
  <xsl:param name="suppress.footer.navigation" select="0"/>
  <xsl:param name="header.rule" select="0"/>
  <xsl:param name="footer.rule" select="1"/>
  <xsl:param name="navig.showtitles" select="1"/>
  <xsl:param name="generate.id.attributes" select="1"/>
  <xsl:param name="highlight.source" select="1"/>
  <xsl:param name="css.decoration" select="0" />
  <xsl:param name="table.borders.with.css" select="0" />
  <xsl:param name="html.ext" select="'.php'" />

  <xsl:param name="generate.toc">
  appendix  toc,title
  article/appendix  nop
  article   toc,title
  book      toc,title
  chapter   nop
  part      nop
  preface   nop
  qandadiv  nop
  qandaset  nop
  reference toc,title
  sect1     nop
  sect2     nop
  sect3     nop
  sect4     nop
  sect5     nop
  section   nop
  set       toc
  </xsl:param>

  <xsl:template match="section[@role = 'NotInToc']"  mode="toc" />

  <xsl:template name="gentext.nav.home">Table of Contents</xsl:template>

  <xsl:template match="figure[@role = 'inline']" mode="class.value">
    <xsl:value-of select="'inlinefigure'"/>
  </xsl:template>
  <xsl:template match="informalfigure[@role = 'inline']" mode="class.value">
    <xsl:value-of select="'inlinefigure'"/>
  </xsl:template>

  <xsl:template name="chunk-element-content">
    <xsl:param name="prev"/>
    <xsl:param name="next"/>
    <xsl:param name="nav.context"/>
    <xsl:param name="content">
      <xsl:apply-imports/>
    </xsl:param>
    <xsl:call-template name="user.preroot"/>
    <xsl:variable name="doc" select="self::*"/>
    <xsl:processing-instruction name="php">
      require_once('<xsl:value-of select="$wordpress.dir"/>/wp-blog-header.php');
      header("HTTP/1.1 200 OK");
      header("Status: 200 All rosy");
      $wp_query->is_404 = false;
      $wp_query->is_home = false;
      $wp_query->is_single = true;
      function my_title_callback(array $title) {
        $title['title'] = "<xsl:apply-templates select="$doc" mode="object.title.markup.textonly"/>";
        return $title;
      }
      add_filter('document_title_parts', 'my_title_callback', 10);
      get_header();
      wp_enqueue_style('ovito-manual-style', get_stylesheet_directory_uri().'/manual-wordpress.css');
    ?</xsl:processing-instruction>
    <div id="content" class="narrowcolumn" role="main">
      <xsl:call-template name="header.navigation">
        <xsl:with-param name="prev" select="$prev"/>
        <xsl:with-param name="next" select="$next"/>
        <xsl:with-param name="nav.context" select="$nav.context"/>
      </xsl:call-template>
    <xsl:call-template name="root.attributes"/>
      <xsl:copy-of select="$content"/>
    </div>
    <xsl:processing-instruction name="php">
      get_footer();
    ?</xsl:processing-instruction>
    <xsl:value-of select="$chunk.append"/>
  </xsl:template>

  <xsl:template name="header.navigation">
    <xsl:param name="prev" select="/foo"/>
    <xsl:param name="next" select="/foo"/>
    <xsl:param name="nav.context"/>

    <xsl:variable name="home" select="/*[1]"/>
    <xsl:variable name="up" select="parent::*"/>

    <xsl:if test="$suppress.navigation = '0' and $suppress.header.navigation = '0'">
      <div class="navheader">
          <table width="100%" summary="Navigation header">
              <tr>
                <xsl:if test="count($up) > 0">
                  <td align="left">
                    <!-- Manual home -->
                    <xsl:if test="$home != . or $nav.context = 'toc'">
                      <a accesskey="h">
                        <xsl:attribute name="href">
                          <xsl:call-template name="href.target">
                            <xsl:with-param name="object" select="$home"/>
                          </xsl:call-template>
                        </xsl:attribute>
                        <xsl:text>OVITO Manual</xsl:text>
                      </a>
                      <xsl:text> &gt; </xsl:text>
                    </xsl:if>

                    <!-- Current location -->
                    <xsl:if test="count($up) > 0
                                    and generate-id($up) != generate-id($home)
                                    and $navig.showtitles != 0">
                      <a accesskey="u">
                        <xsl:attribute name="href">
                          <xsl:call-template name="href.target">
                            <xsl:with-param name="object" select="$up"/>
                          </xsl:call-template>
                        </xsl:attribute>
                        <xsl:apply-templates select="$up" mode="object.title.markup"/>
                      </a>
                      <xsl:text> &gt; </xsl:text>
                    </xsl:if>
                    <xsl:apply-templates select="." mode="object.title.markup"/>
                  </td>
                </xsl:if>

                <td>
                  <!-- Version -->
                  <xsl:text>Release </xsl:text>
                  <xsl:value-of select="$ovito.version"/>
                </td>

                <td align="right">
                  <!-- Prev -->
                  <xsl:if test="count($prev)>0">
                    <a accesskey="p">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$prev"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'prev'"/>
                      </xsl:call-template>
                    </a>
                    <xsl:text> | </xsl:text>
                  </xsl:if>

                  <!-- Next -->
                  <xsl:if test="count($next)>0">
                    <a accesskey="n">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$next"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'next'"/>
                      </xsl:call-template>
                    </a>
                  </xsl:if>

                </td>
              </tr>
          </table>
      </div>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>