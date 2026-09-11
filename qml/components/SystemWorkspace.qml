import QtQuick

Item {
    id: root

    required property var engine
    property bool loadRequested: false

    // P5_LAZY_SYSTEM_WORKSPACE_V1
    // System is the largest non-EQ workspace. Defer its object tree until the
    // user visits System for the first time, then keep it alive for the rest of
    // the session to avoid navigation churn. Fixed EQ page/model ownership is
    // intentionally untouched.
    onVisibleChanged: {
        if (visible)
            loadRequested = true
    }

    Component.onCompleted: {
        if (visible)
            loadRequested = true
    }

    Loader {
        id: systemWorkspaceLoader
        anchors.fill: parent
        active: root.loadRequested
        asynchronous: true

        sourceComponent: Component {
            SystemWorkspaceImpl {
                engine: root.engine
            }
        }
    }
}
