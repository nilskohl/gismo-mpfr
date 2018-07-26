/** @file gsLobattoRule.hpp

    @brief Provides implementation of the Gauss-Lobatto quadrature rule

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): A. Mantzaflaris
*/

#pragma once

#include <gsCore/gsBasis.h>
#include <gsIO/gsOptionList.h>

namespace gismo
{

template<class T> void
gsLobattoRule<T>::setNodes( gsVector<index_t> const & numNodes,
                            unsigned digits)
{
    const int d = numNodes.rows();
    static const T epsilon = std::pow(10.0, -REAL_DIG * 0.85);
    // Get base rule nodes and weights
    std::vector<gsVector<T> > nodes(d);
    std::vector<gsVector<T> > weights(d);

    if (digits == 0)
    {
        for (int i = 0; i < d; ++i)
        {
            if (!lookupReference(numNodes[i], nodes[i], weights[i]))
                computeReference(numNodes[i], nodes[i], weights[i], REAL_DIG);
            if (1!=numNodes[i])
                nodes[i].last() -= epsilon; //interval may be half-open
        }
    }
    else
    {
        for (int i = 0; i < d; ++i)
        {
            computeReference(numNodes[i], nodes[i], weights[i], digits);
            if (1!=numNodes[i])
                nodes[i].last() -= epsilon; //interval may be half-open
        }
    }

    //std::copy(nodes.begin(), nodes.end(), std::ostream_iterator<gsVector<T> >(gsInfo, "\n"));

    this->computeTensorProductRule(nodes, weights);
}

template<class T> void
gsLobattoRule<T>::computeReference(index_t n,       // Number of points
                                   gsVector<T> & x, // Quadrature points
                                   gsVector<T> & w, // Quadrature weights
                                   unsigned digits) // Number of exact decimal digits
{
    GISMO_ASSERT(digits!=0, "Digits cannot be 0");
    // Allocate space for points and weights.
    x.resize(n);
    w.resize(n);

    int i, j;
    T test;
    const T tolerance = math::pow(T(0.1), static_cast<int>(digits));

    if ( n == 1 )
    {
        x[0] = -1.0;
        w[0] = 2.0;
        return;
    }

    // Initial estimate ( Chebyshev-Gauss-Lobatto nodes)
    for ( i = 0; i < n; i++ )
        x[i] = math::cos ( EIGEN_PI * ( i ) / ( n - 1 ) );

    gsVector<T> xold(n);
    gsVector<T> p(n*n);

    do
    {
        for ( i = 0; i < n; i++ )
        {
            xold[i] = p[i+1*n] = x[i];
            p[i+0*n] = 1.0;
        }

        for ( j = 2; j <= n-1; j++ )
        {
            for ( i = 0; i < n; i++)
            {
                p[i+j*n] = ( (T) ( 2 * j - 1 ) * x[i] * p[i+(j-1)*n]
                             + (T) (   - j + 1 ) *        p[i+(j-2)*n] )
                    / (T) (     j     );
            }
        }

        for ( i = 0; i < n; i++ )
            x[i] = xold[i] - ( x[i] * p[i+(n-1)*n] - p[i+(n-2)*n] )
                / ( (T) ( n ) * p[i+(n-1)*n] );

        test = 0.0;
        for ( i = 0; i < n; i++ )
            test = math::max( test, math::abs ( x[i] - xold[i] ) );

    } while ( tolerance <= test );

    x.reverseInPlace();

    for ( i = 0; i < n; i++ )
        w[i] = 2.0 / ( (T) ( ( n - 1 ) * n ) * math::pow ( p[i+(n-1)*n], 2 ) );
}


template<class T> bool
gsLobattoRule<T>::lookupReference(index_t n,   // Number of points
                                  gsVector<T> & x, // Quadrature points
                                  gsVector<T> & w) // Quadrature weights
{
    x.resize(n);
    w.resize(n);

    switch (n)
    {
    case 1 :
    {
        x <<  0.0; // Note was: -1.0, but we better use 0
        w <<  2.0;
        return true;
    }
    case 2 :
    {
        x <<  - 1.0E+00, 1.0E+00;
        w <<  1.0E+00, 1.0E+00;
        return true;
    }
    case 3 :
    {
        x <<  - 1.0E+00, 0.0E+00, 1.0E+00;
        w <<  1.0 / 3.0E+00, 4.0 / 3.0E+00, 1.0 / 3.0E+00;
        return true;
    }
    case 4 :
    {
        x <<  - 1.0E+00,- 0.447213595499957939281834733746E+00, 0.447213595499957939281834733746E+00, 1.0E+00;
        w << 1.0E+00 / 6.0E+00, 5.0E+00 / 6.0E+00, 5.0E+00 / 6.0E+00, 1.0E+00 / 6.0E+00;
        return true;
    }
    case 5 :
    {
        x << - 1.0E+00, - 0.654653670707977143798292456247E+00, 0.0E+00, 0.654653670707977143798292456247E+00, 1.0E+00;
        w << 9.0E+00 / 90.0E+00, 49.0E+00 / 90.0E+00, 64.0E+00 / 90.0E+0, 49.0E+00 / 90.0E+00, 9.0E+00 / 90.0E+00;
        return true;
    }
    case 6 :
    {
        x << - 1.0E+00,- 0.765055323929464692851002973959E+00,- 0.285231516480645096314150994041E+00, 0.285231516480645096314150994041E+00, 0.765055323929464692851002973959E+00, 1.0E+00;
        w <<  0.066666666666666666666666666667E+00, 0.378474956297846980316612808212E+00, 0.554858377035486353016720525121E+00, 0.554858377035486353016720525121E+00,  0.378474956297846980316612808212E+00, 0.066666666666666666666666666667E+00;
        return true;
    }
    case 7 :
    {
        x << - 1.0E+00, - 0.830223896278566929872032213967E+00, - 0.468848793470714213803771881909E+00, 0.0E+00, 0.468848793470714213803771881909E+00, 0.830223896278566929872032213967E+00, 1.0E+00;
        w << 0.476190476190476190476190476190E-01, 0.276826047361565948010700406290E+00, 0.431745381209862623417871022281E+00, 0.487619047619047619047619047619E+00, 0.431745381209862623417871022281E+00, 0.276826047361565948010700406290E+00, 0.476190476190476190476190476190E-01;
        return true;
    }
    case 8 :
    {
        x <<  - 1.0E+00, - 0.871740148509606615337445761221E+00, - 0.591700181433142302144510731398E+00, - 0.209299217902478868768657260345E+00, 0.209299217902478868768657260345E+00, 0.591700181433142302144510731398E+00, 0.871740148509606615337445761221E+00, 1.0E+00;
        w <<  0.357142857142857142857142857143E-01, 0.210704227143506039382991065776E+00, 0.341122692483504364764240677108E+00, 0.412458794658703881567052971402E+00,  0.412458794658703881567052971402E+00, 0.341122692483504364764240677108E+00, 0.210704227143506039382991065776E+00, 0.357142857142857142857142857143E-01;
        return true;
    }

    }

    switch (n)
    {
    case 9 :
    {
        x << - 1.0E+00,  - 0.899757995411460157312345244418E+00, - 0.677186279510737753445885427091E+00, - 0.363117463826178158710752068709E+00,   0.0E+00, 0.363117463826178158710752068709E+00,   0.677186279510737753445885427091E+00,   0.899757995411460157312345244418E+00,   1.0E+00;

        w <<  0.277777777777777777777777777778E-01, 0.165495361560805525046339720029E+00, 0.274538712500161735280705618579E+00, 0.346428510973046345115131532140E+00, 0.371519274376417233560090702948E+00, 0.346428510973046345115131532140E+00, 0.274538712500161735280705618579E+00, 0.165495361560805525046339720029E+00, 0.277777777777777777777777777778E-01;
        return true;
    }
    case 10 :
    {
        x <<  - 1.0E+00,  - 0.919533908166458813828932660822E+00, - 0.738773865105505075003106174860E+00, - 0.477924949810444495661175092731E+00, - 0.165278957666387024626219765958E+00,    0.165278957666387024626219765958E+00,   0.477924949810444495661175092731E+00,   0.738773865105505075003106174860E+00,   0.919533908166458813828932660822E+00,  1.0E+00;

        w <<  0.222222222222222222222222222222E-01,  0.133305990851070111126227170755E+00,  0.224889342063126452119457821731E+00,  0.292042683679683757875582257374E+00,  0.327539761183897456656510527917E+00,  0.327539761183897456656510527917E+00,  0.292042683679683757875582257374E+00,  0.224889342063126452119457821731E+00,  0.133305990851070111126227170755E+00,  0.222222222222222222222222222222E-01;
        return true;
    }
    case 11 :
    {
        x <<  - 1.0E+00,  - 0.934001430408059134332274136099E+00, - 0.784483473663144418622417816108E+00, - 0.565235326996205006470963969478E+00, - 0.295758135586939391431911515559E+00,    0.0E+00,   0.295758135586939391431911515559E+00,   0.565235326996205006470963969478E+00,   0.784483473663144418622417816108E+00,  0.934001430408059134332274136099E+00,  1.0E+00;

        w << 0.181818181818181818181818181818E-01,  0.109612273266994864461403449580E+00,  0.187169881780305204108141521899E+00,  0.248048104264028314040084866422E+00,  0.286879124779008088679222403332E+00,  0.300217595455690693785931881170E+00,  0.286879124779008088679222403332E+00,  0.248048104264028314040084866422E+00,  0.187169881780305204108141521899E+00,  0.109612273266994864461403449580E+00,  0.181818181818181818181818181818E-01;

        return true;
    }
    case 12 :
    {
        x <<  - 1.0E+00,  - 0.944899272222882223407580138303E+00, - 0.819279321644006678348641581717E+00, - 0.632876153031869677662404854444E+00, - 0.399530940965348932264349791567E+00,  - 0.136552932854927554864061855740E+00,   0.136552932854927554864061855740E+00,   0.399530940965348932264349791567E+00,   0.632876153031869677662404854444E+00,  0.819279321644006678348641581717E+00,  0.944899272222882223407580138303E+00,  1.0E+00;

        w <<  0.151515151515151515151515151515E-01,   0.916845174131961306683425941341E-01,   0.157974705564370115164671062700E+00,   0.212508417761021145358302077367E+00,   0.251275603199201280293244412148E+00,   0.271405240910696177000288338500E+00,   0.271405240910696177000288338500E+00,   0.251275603199201280293244412148E+00,   0.212508417761021145358302077367E+00,   0.157974705564370115164671062700E+00,   0.916845174131961306683425941341E-01,   0.151515151515151515151515151515E-01;
        return true;
    }
    case 13 :
    {
        x << - 1.0E+00,  - 0.953309846642163911896905464755E+00, - 0.846347564651872316865925607099E+00, - 0.686188469081757426072759039566E+00, - 0.482909821091336201746937233637E+00,  - 0.249286930106239992568673700374E+00,   0.0E+00,   0.249286930106239992568673700374E+00,   0.482909821091336201746937233637E+00,  0.686188469081757426072759039566E+00,  0.846347564651872316865925607099E+00,  0.953309846642163911896905464755E+00, 1.0E+00;

        w <<  0.128205128205128205128205128205E-01,   0.778016867468189277935889883331E-01,   0.134981926689608349119914762589E+00,   0.183646865203550092007494258747E+00,   0.220767793566110086085534008379E+00,   0.244015790306676356458578148360E+00,   0.251930849333446736044138641541E+00,   0.244015790306676356458578148360E+00,   0.220767793566110086085534008379E+00,   0.183646865203550092007494258747E+00,   0.134981926689608349119914762589E+00,   0.778016867468189277935889883331E-01,   0.128205128205128205128205128205E-01;
        return true;
    }
    case 14 :
    {
        x << - 1.0E+00,  - 0.959935045267260901355100162015E+00, - 0.867801053830347251000220202908E+00, - 0.728868599091326140584672400521E+00, - 0.550639402928647055316622705859E+00,  - 0.342724013342712845043903403642E+00, - 0.116331868883703867658776709736E+00,   0.116331868883703867658776709736E+00,   0.342724013342712845043903403642E+00,  0.550639402928647055316622705859E+00,  0.728868599091326140584672400521E+00,  0.867801053830347251000220202908E+00, 0.959935045267260901355100162015E+00, 1.0E+00;

        w <<  0.109890109890109890109890109890E-01,   0.668372844976812846340706607461E-01,   0.116586655898711651540996670655E+00,   0.160021851762952142412820997988E+00,   0.194826149373416118640331778376E+00,   0.219126253009770754871162523954E+00,   0.231612794468457058889628357293E+00,   0.231612794468457058889628357293E+00,   0.219126253009770754871162523954E+00,  0.194826149373416118640331778376E+00,  0.160021851762952142412820997988E+00,  0.116586655898711651540996670655E+00,  0.668372844976812846340706607461E-01,  0.109890109890109890109890109890E-01;
        return true;
    }
    case 15 :
    {
        x <<  - 1.0E+00,  - 0.965245926503838572795851392070E+00, - 0.885082044222976298825401631482E+00, - 0.763519689951815200704118475976E+00, - 0.606253205469845711123529938637E+00,  - 0.420638054713672480921896938739E+00, - 0.215353955363794238225679446273E+00,   0.0E+00,   0.215353955363794238225679446273E+00,  0.420638054713672480921896938739E+00,  0.606253205469845711123529938637E+00,  0.763519689951815200704118475976E+00, 0.885082044222976298825401631482E+00, 0.965245926503838572795851392070E+00, 1.0E+00;

        w <<  0.952380952380952380952380952381E-02,   0.580298930286012490968805840253E-01,   0.101660070325718067603666170789E+00,   0.140511699802428109460446805644E+00,   0.172789647253600949052077099408E+00,   0.196987235964613356092500346507E+00,   0.211973585926820920127430076977E+00,   0.217048116348815649514950214251E+00,   0.211973585926820920127430076977E+00,  0.196987235964613356092500346507E+00,  0.172789647253600949052077099408E+00,  0.140511699802428109460446805644E+00,  0.101660070325718067603666170789E+00,  0.580298930286012490968805840253E-01,  0.952380952380952380952380952381E-02;
        return true;
    }
    case 16 :
    {
        x <<  - 1.0E+00,  - 0.969568046270217932952242738367E+00, - 0.899200533093472092994628261520E+00, - 0.792008291861815063931088270963E+00, - 0.652388702882493089467883219641E+00,  - 0.486059421887137611781890785847E+00, - 0.299830468900763208098353454722E+00, - 0.101326273521949447843033005046E+00,   0.101326273521949447843033005046E+00,  0.299830468900763208098353454722E+00,  0.486059421887137611781890785847E+00,  0.652388702882493089467883219641E+00, 0.792008291861815063931088270963E+00, 0.899200533093472092994628261520E+00, 0.969568046270217932952242738367E+00, 1.0E+00;

        w <<  0.833333333333333333333333333333E-02,   0.508503610059199054032449195655E-01,   0.893936973259308009910520801661E-01,   0.124255382132514098349536332657E+00,   0.154026980807164280815644940485E+00,   0.177491913391704125301075669528E+00,   0.193690023825203584316913598854E+00,   0.201958308178229871489199125411E+00,   0.201958308178229871489199125411E+00,  0.193690023825203584316913598854E+00,  0.177491913391704125301075669528E+00,  0.154026980807164280815644940485E+00,  0.124255382132514098349536332657E+00,  0.893936973259308009910520801661E-01,  0.508503610059199054032449195655E-01,  0.833333333333333333333333333333E-02;
        return true;
    }
    case 17 :
    {
        x <<  - 1.0E+00,  - 0.973132176631418314156979501874E+00, - 0.910879995915573595623802506398E+00, - 0.815696251221770307106750553238E+00, - 0.691028980627684705394919357372E+00,  - 0.541385399330101539123733407504E+00, - 0.372174433565477041907234680735E+00, - 0.189511973518317388304263014753E+00,   0.0E+00,  0.189511973518317388304263014753E+00,  0.372174433565477041907234680735E+00,  0.541385399330101539123733407504E+00, 0.691028980627684705394919357372E+00, 0.815696251221770307106750553238E+00, 0.910879995915573595623802506398E+00, 0.973132176631418314156979501874E+00, 1.0E+00;

        w << 0.735294117647058823529411764706E-02,   0.449219405432542096474009546232E-01,   0.791982705036871191902644299528E-01,   0.110592909007028161375772705220E+00,   0.137987746201926559056201574954E+00,   0.160394661997621539516328365865E+00,   0.177004253515657870436945745363E+00,   0.187216339677619235892088482861E+00,   0.190661874753469433299407247028E+00,  0.187216339677619235892088482861E+00,  0.177004253515657870436945745363E+00,  0.160394661997621539516328365865E+00,  0.137987746201926559056201574954E+00,  0.110592909007028161375772705220E+00,  0.791982705036871191902644299528E-01,  0.449219405432542096474009546232E-01,  0.735294117647058823529411764706E-02;
        return true;
    }
    case 18 :
    {
        x << - 1.0E+00,  - 0.976105557412198542864518924342E+00, - 0.920649185347533873837854625431E+00, - 0.835593535218090213713646362328E+00, - 0.723679329283242681306210365302E+00,  - 0.588504834318661761173535893194E+00, - 0.434415036912123975342287136741E+00, - 0.266362652878280984167665332026E+00, - 0.897490934846521110226450100886E-01,  0.897490934846521110226450100886E-01,  0.266362652878280984167665332026E+00,  0.434415036912123975342287136741E+00,0.588504834318661761173535893194E+00,0.723679329283242681306210365302E+00,0.835593535218090213713646362328E+00,0.920649185347533873837854625431E+00,0.976105557412198542864518924342E+00,1.0E+00;

        w <<  0.653594771241830065359477124183E-02,   0.399706288109140661375991764101E-01,   0.706371668856336649992229601678E-01,   0.990162717175028023944236053187E-01,   0.124210533132967100263396358897E+00,   0.145411961573802267983003210494E+00,   0.161939517237602489264326706700E+00,   0.173262109489456226010614403827E+00,   0.179015863439703082293818806944E+00,  0.179015863439703082293818806944E+00,  0.173262109489456226010614403827E+00,  0.161939517237602489264326706700E+00,  0.145411961573802267983003210494E+00,  0.124210533132967100263396358897E+00,  0.990162717175028023944236053187E-01,  0.706371668856336649992229601678E-01,  0.399706288109140661375991764101E-01,  0.653594771241830065359477124183E-02;
        return true;
    }
    case 19 :
    {
        x <<  - 1.0E+00,  - 0.978611766222080095152634063110E+00, - 0.928901528152586243717940258797E+00, - 0.852460577796646093085955970041E+00, - 0.751494202552613014163637489634E+00,  - 0.628908137265220497766832306229E+00, - 0.488229285680713502777909637625E+00, - 0.333504847824498610298500103845E+00, - 0.169186023409281571375154153445E+00,  0.0E+00,  0.169186023409281571375154153445E+00,  0.333504847824498610298500103845E+00,0.488229285680713502777909637625E+00,0.628908137265220497766832306229E+00,0.751494202552613014163637489634E+00,0.852460577796646093085955970041E+00,0.928901528152586243717940258797E+00,0.978611766222080095152634063110E+00,1.0E+00;

        w <<  0.584795321637426900584795321637E-02,   0.357933651861764771154255690351E-01,   0.633818917626297368516956904183E-01,   0.891317570992070844480087905562E-01,   0.112315341477305044070910015464E+00,   0.132267280448750776926046733910E+00,   0.148413942595938885009680643668E+00,   0.160290924044061241979910968184E+00,   0.167556584527142867270137277740E+00,  0.170001919284827234644672715617E+00,  0.167556584527142867270137277740E+00,  0.160290924044061241979910968184E+00,  0.148413942595938885009680643668E+00,  0.132267280448750776926046733910E+00,  0.112315341477305044070910015464E+00,  0.891317570992070844480087905562E-01,  0.633818917626297368516956904183E-01,  0.357933651861764771154255690351E-01,  0.584795321637426900584795321637E-02;
        return true;
    }
    case 20 :
    {
        x << - 1.0E+00,  - 0.980743704893914171925446438584E+00, - 0.935934498812665435716181584931E+00, - 0.866877978089950141309847214616E+00, - 0.775368260952055870414317527595E+00,  - 0.663776402290311289846403322971E+00, - 0.534992864031886261648135961829E+00, - 0.392353183713909299386474703816E+00, - 0.239551705922986495182401356927E+00,- 0.805459372388218379759445181596E-01,  0.805459372388218379759445181596E-01,  0.239551705922986495182401356927E+00,0.392353183713909299386474703816E+00,0.534992864031886261648135961829E+00,0.663776402290311289846403322971E+00,0.775368260952055870414317527595E+00,0.866877978089950141309847214616E+00,0.935934498812665435716181584931E+00,0.980743704893914171925446438584E+00, 1.0E+00;

           w <<  0.526315789473684210526315789474E-02, 0.322371231884889414916050281173E-01,   0.571818021275668260047536271732E-01,   0.806317639961196031447768461137E-01,   0.101991499699450815683781205733E+00,   0.120709227628674725099429705002E+00,   0.136300482358724184489780792989E+00,   0.148361554070916825814713013734E+00,   0.156580102647475487158169896794E+00,  0.160743286387845749007726726449E+00,  0.160743286387845749007726726449E+00,  0.156580102647475487158169896794E+00,  0.148361554070916825814713013734E+00,  0.136300482358724184489780792989E+00,  0.120709227628674725099429705002E+00,  0.101991499699450815683781205733E+00,  0.806317639961196031447768461137E-01,  0.571818021275668260047536271732E-01,  0.322371231884889414916050281173E-01,  0.526315789473684210526315789474E-02;
        return true;
    }
    default:
    {
        //gsWarn << "  Illegal value of N = " << n << "\n";
        gsWarn << "Precomputed Lobatto rule (1,..,20) not found.\n";
        return false;
    }

    }//switch

}// lookupReference

} // namespace gismo
